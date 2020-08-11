
#include "CacheManager.h"
#include "RawFileInterface.h"
#include "TypedCacheManager.h"
#include "LogPage.h"

#include <assert.h>
#include <algorithm>
#include <tuple>
#include <iterator>

using namespace TxFs;

CacheManager::CacheManager(RawFileInterface* rfi, uint32_t maxPages) noexcept
    : m_rawFileInterface(rfi)
    , m_pageMemoryAllocator(maxPages)
    , m_maxCachedPages(maxPages)
{}

/// Delivers a new page. The page is either allocated form the FreeStore or it comes from extending the file. The
/// PageDef<> is writable as it is expected that a new page was requested because you want to write to it.
PageDef<uint8_t> CacheManager::newPage()
{
    auto page = m_pageMemoryAllocator.allocate();
    auto id = newPageIndex();
    m_cache.emplace(id, CachedPage(page, PageClass::New));
    m_newPageSet.insert(id);
    trimCheck();
    return PageDef<uint8_t>(page, id);
}

/// Loads the specified page. The page is loaded because previous transactions left state that is now being accessed.
/// The return value can be transformed into something writable (makePageWritable()) which in turn makes this page
/// subject to the dirty-page protocol.
ConstPageDef<uint8_t> CacheManager::loadPage(PageIndex origId)
{
    auto id = redirectPage(origId);
    auto it = m_cache.find(id);
    if (it == m_cache.end())
    {
        auto page = m_pageMemoryAllocator.allocate();
        TxFs::readPage(m_rawFileInterface, id, page.get());
        m_cache.emplace(id, CachedPage(page, PageClass::Read));
        trimCheck();
        return ConstPageDef<uint8_t>(page, origId);
    }

    it->second.m_usageCount++;                               
    return ConstPageDef<uint8_t>(it->second.m_page, origId);
}

/// Reuses a page for new purposes. It works like loadPage() without physically loading the page, followed by
/// setPageDirty(). The page is treated as PageClass::New if we find the page in the m_newPageSet otherwise it will
/// be flagged as PageClass::Dirty. Note: Do not feed regular FreeStore pages to this API as they wrongly end up
/// following the dirty-page protocol.
PageDef<uint8_t> CacheManager::repurpose(PageIndex origId)
{
    auto id = redirectPage(origId);
    PageClass pageClass = m_newPageSet.count(id) ? PageClass::New : PageClass::Dirty;
    auto it = m_cache.find(id);
    if (it == m_cache.end())
    {
        auto page = m_pageMemoryAllocator.allocate();
        m_cache.emplace(id, CachedPage(page, pageClass));
        trimCheck();
        return PageDef<uint8_t>(page, origId);
    }

    it->second.m_usageCount++;
    it->second.setPageClass(pageClass);
    return PageDef<uint8_t>(it->second.m_page, origId);
}

/// Transforms a const page into a writable page. CacheManager needs to know that pages written by a previous
/// transaction are now about to be changed. Such pages are subject to the dirty-page protocol.
PageDef<uint8_t> CacheManager::makePageWritable(const ConstPageDef<uint8_t>& loadedPage) noexcept
{
    setPageDirty(loadedPage.m_index);
    return PageDef<uint8_t>(std::const_pointer_cast<uint8_t>(loadedPage.m_page), loadedPage.m_index);
}

/// Marks that a page was changed: Pages previously read-in are marked dirty (which makes them follow the
/// dirty-page protocoll). All other pages are treated as PageClass::New.
void CacheManager::setPageDirty(PageIndex id) noexcept
{
    id = redirectPage(id);
    PageClass pageClass = m_newPageSet.count(id) ? PageClass::New : PageClass::Dirty;

    auto it = m_cache.find(id);
    assert(it != m_cache.end());
    it->second.setPageClass(pageClass);
}

/// Finds out if a trim operation needs to be performed and does it if necessary.
void CacheManager::trimCheck() noexcept
{
    if (m_cache.size() > m_maxCachedPages)
        trim(m_maxCachedPages / 4 * 3);
}

// Trims down memory usage by maxPages. If users have a lot of pinned pages this is triggered too often. Make sure that
// there is sufficient space to deal with real-world scenarios.
size_t CacheManager::trim(uint32_t maxPages)
{
    auto prioritizedPages = getUnpinnedPages();
    maxPages = std::min(maxPages, (uint32_t) prioritizedPages.size());
    auto beginEvictSet = prioritizedPages.begin() + maxPages;

    std::nth_element(prioritizedPages.begin(), beginEvictSet, prioritizedPages.end());
    auto beginNewPageSet = std::partition(beginEvictSet, prioritizedPages.end(),
                                          [](PrioritizedPage psi) { return psi.m_pageClass == PageClass::Dirty; });
    auto endNewPageSet = std::partition(beginNewPageSet, prioritizedPages.end(),
                                        [](PrioritizedPage psi) { return psi.m_pageClass == PageClass::New; });

    evictDirtyPages(beginEvictSet, beginNewPageSet);
    evictNewPages(beginNewPageSet, endNewPageSet);
    removeFromCache(beginEvictSet, prioritizedPages.end());
    return m_cache.size();
}

/// Use installed allocation function or the rawFileInterface.
Interval CacheManager::allocatePageInterval(size_t maxPages) noexcept
{
    if (m_pageIntervalAllocator)
    {
        auto iv = m_pageIntervalAllocator(maxPages);
        if (iv.begin() == PageIdx::INVALID)
            m_pageIntervalAllocator = std::function<Interval(size_t)>();
        return iv;
    }
    return m_rawFileInterface->newInterval(maxPages);
}

/// Get the page-indices for redirected Dirty pages
std::vector<PageIndex> CacheManager::getRedirectedPages() const
{
    std::vector<PageIndex> pages;
    pages.reserve(m_redirectedPagesMap.size());
    for (const auto& [originalPage, redirectedPage]: m_redirectedPagesMap)
        pages.push_back(redirectedPage);

    return pages;
}

/// Find the page we moved the original page to or return identity.
PageIndex CacheManager::redirectPage(PageIndex id) const noexcept
{
    auto it = m_redirectedPagesMap.find(id);
    if (it == m_redirectedPagesMap.end())
        return id;
    return it->second;
}

/// Find all pages that are currently not pinned.
std::vector<CacheManager::PrioritizedPage> CacheManager::getUnpinnedPages() const
{
    std::vector<PrioritizedPage> pageSortItems;
    pageSortItems.reserve(m_cache.size());
    for (auto& cp: m_cache)
        if (cp.second.m_page.use_count() == 1) // we don't use weak_ptr => this is save
            pageSortItems.emplace_back(cp.second, cp.first);
    return pageSortItems;
}

void CacheManager::evictDirtyPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        assert(it->m_pageClass == PageClass::Dirty);
        auto p = m_cache.find(it->m_id);
        assert(p != m_cache.end());
        auto id = newPageIndex();
        TxFs::writePage(m_rawFileInterface, id, p->second.m_page.get());
        m_redirectedPagesMap[it->m_id] = id;
        m_newPageSet.insert(id);
    }
}

void CacheManager::evictNewPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        assert(it->m_pageClass == PageClass::New);
        auto p = m_cache.find(it->m_id);
        assert(p != m_cache.end());
        TxFs::writePage(m_rawFileInterface, p->first, p->second.m_page.get());
    }
}

void CacheManager::removeFromCache(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end)
{
    for (auto it = begin; it != end; ++it)
        m_cache.erase(it->m_id);
}

void CacheManager::commit()
{
    // stop allocating from FreeStore
    m_pageIntervalAllocator = std::function<Interval(size_t)>();

    auto dirtyPageIds = getAllDirtyPageIds();
    {
        // make sure the copies are visible before the Logs
        auto origToCopyPages = copyDirtyPages(dirtyPageIds);
        m_rawFileInterface->flushFile();

        // make sure the Logs are visible before we overwrite original contents
        writeLogs(origToCopyPages);
        m_rawFileInterface->flushFile();
    }

    updateDirtyPages(dirtyPageIds);
    writeCachedPages();
    m_rawFileInterface->flushFile();

    // cut the file
}

/// Get the original ids of the PageClass::Dirty pages. Some of them may
/// still live in m_cache the others were probably pushed out by the
/// dirty-page eviction protocol.
std::vector<PageIndex> CacheManager::getAllDirtyPageIds() const
{
    std::vector<PageIndex> dirtyPageIds;
    dirtyPageIds.reserve(m_redirectedPagesMap.size());
    for (const auto& [originalPageIdx, redirectedPageIdx]: m_redirectedPagesMap)
        dirtyPageIds.push_back(originalPageIdx);

    for (auto& p: m_cache)
        if (p.second.m_pageClass == PageClass::Dirty)
            dirtyPageIds.push_back(p.first);

    return dirtyPageIds;
}

/// Make a copy of the unmodified dirty pages. The original state of these pages
/// is not in the cache so we just copy from the file to a new location in the file.
std::vector<std::pair<PageIndex, PageIndex>> CacheManager::copyDirtyPages(const std::vector<PageIndex>& dirtyPageIds)
{
    std::vector<std::pair<PageIndex, PageIndex>> origToCopyPages;
    origToCopyPages.reserve(dirtyPageIds.size());

    auto interval = m_rawFileInterface->newInterval(dirtyPageIds.size());
    assert(interval.length() == dirtyPageIds.size()); // here the file is just growing
    auto nextPage = interval.begin();

    for (auto originalPageIdx: dirtyPageIds)
    {
        TxFs::copyPage(m_rawFileInterface, originalPageIdx, nextPage);
        origToCopyPages.emplace_back(originalPageIdx, nextPage++);
    }
    assert(nextPage == interval.end());

    return origToCopyPages;
}

/// Update the original PageClass::DirtyPage pages either from the cache or from the redirected
/// pages and erase them from the cache.
void CacheManager::updateDirtyPages(const std::vector<PageIndex>& dirtyPageIds)
{
    for (auto origIdx : dirtyPageIds)
    {
        auto id = redirectPage(origIdx);
        auto it = m_cache.find(id);
        if (it == m_cache.end()) 
        {
            // if the page is not in the cache just physically copy the page from
            // its redirected place. (PageClass::Dirty pages are either in the cache or redirected)
            assert(id != origIdx); 
            TxFs::copyPage(m_rawFileInterface, id, origIdx);
        }
        else
        {
            // we have to use the cached page or else we lose updates (if the page is not PageClass::Read)!
            TxFs::writePage(m_rawFileInterface, origIdx, it->second.m_page.get());
            m_cache.erase(it);
        }
    }
}

/// Pages that are still in the cache are written to the file. 
void CacheManager::writeCachedPages()
{
    for (const auto& page: m_cache)
    {
        assert(page.second.m_pageClass != PageClass::Undefined);
        if (page.second.m_pageClass != PageClass::Read)
            TxFs::writePage(m_rawFileInterface, page.first, page.second.m_page.get());
    }
    m_cache.clear();
}

/// Fill the log pages with data and write them to the file. 
void CacheManager::writeLogs(const std::vector<std::pair<PageIndex, PageIndex>>& origToCopyPages)
{
    auto begin = origToCopyPages.begin();
    while (begin != origToCopyPages.end())
    {
        auto pageIndex = m_rawFileInterface->newInterval(1).begin();
        LogPage logPage(pageIndex);
        begin = logPage.pushBack(begin, origToCopyPages.end());
        TxFs::writePage(m_rawFileInterface, pageIndex, &logPage);

    }
}

std::vector<std::pair<PageIndex, PageIndex>> CacheManager::readLogs() const
{
    std::vector<std::pair<PageIndex, PageIndex>> res;
    auto size = m_rawFileInterface->currentSize();
    if (!size)
        return res;

    PageIndex idx = static_cast<PageIndex>(size);
    LogPage logPage{};

    do 
    {
        TxFs::readPage(m_rawFileInterface, --idx, &logPage);
        if (!logPage.checkSignature(idx))
            return res;

        res.reserve(res.size() + logPage.size());
        for (auto [orig, cpy]: logPage)
            res.emplace_back(orig, cpy);
    } while (idx != 0);

    return res;
}