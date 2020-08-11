
#include "CacheManager.h"
#include "RawFileInterface.h"
#include "TypedCacheManager.h"
#include "LogPage.h"
#include "CommitHandler.h"

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
    m_pageCache.emplace(id, CachedPage(page, PageClass::New));
    m_newPageIds.insert(id);
    trimCheck();
    return PageDef<uint8_t>(page, id);
}

/// Loads the specified page. The page is loaded because previous transactions left state that is now being accessed.
/// The return value can be transformed into something writable (makePageWritable()) which in turn makes this page
/// subject to the dirty-page protocol.
ConstPageDef<uint8_t> CacheManager::loadPage(PageIndex origId)
{
    auto id = divertPage(origId);
    auto it = m_pageCache.find(id);
    if (it == m_pageCache.end())
    {
        auto page = m_pageMemoryAllocator.allocate();
        TxFs::readPage(m_rawFileInterface, id, page.get());
        m_pageCache.emplace(id, CachedPage(page, PageClass::Read));
        trimCheck();
        return ConstPageDef<uint8_t>(page, origId);
    }

    it->second.m_usageCount++;                               
    return ConstPageDef<uint8_t>(it->second.m_page, origId);
}

/// Reuses a page for new purposes. It works like loadPage() without physically loading the page, followed by
/// setPageDirty(). The page is treated as PageClass::New if we find the page in the m_newPageIds otherwise it will
/// be flagged as PageClass::Dirty. Note: Do not feed regular FreeStore pages to this API as they wrongly end up
/// following the dirty-page protocol.
PageDef<uint8_t> CacheManager::repurpose(PageIndex origId)
{
    auto id = divertPage(origId);
    PageClass pageClass = m_newPageIds.count(id) ? PageClass::New : PageClass::Dirty;
    auto it = m_pageCache.find(id);
    if (it == m_pageCache.end())
    {
        auto page = m_pageMemoryAllocator.allocate();
        m_pageCache.emplace(id, CachedPage(page, pageClass));
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
    id = divertPage(id);
    PageClass pageClass = m_newPageIds.count(id) ? PageClass::New : PageClass::Dirty;

    auto it = m_pageCache.find(id);
    assert(it != m_pageCache.end());
    it->second.setPageClass(pageClass);
}

/// Finds out if a trim operation needs to be performed and does it if necessary.
void CacheManager::trimCheck() noexcept
{
    if (m_pageCache.size() > m_maxCachedPages)
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
    return m_pageCache.size();
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

/// Find the page we moved the original page to or return identity.
PageIndex CacheManager::divertPage(PageIndex id) const noexcept
{
    auto it = m_divertedPageIds.find(id);
    if (it == m_divertedPageIds.end())
        return id;
    return it->second;
}

/// Find all pages that are currently not pinned.
std::vector<PrioritizedPage> CacheManager::getUnpinnedPages() const
{
    std::vector<PrioritizedPage> pageSortItems;
    pageSortItems.reserve(m_pageCache.size());
    for (auto& cp: m_pageCache)
        if (cp.second.m_page.use_count() == 1) // we don't use weak_ptr => this is save
            pageSortItems.emplace_back(cp.second, cp.first);
    return pageSortItems;
}

void CacheManager::evictDirtyPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        assert(it->m_pageClass == PageClass::Dirty);
        auto p = m_pageCache.find(it->m_id);
        assert(p != m_pageCache.end());
        auto id = newPageIndex();
        TxFs::writePage(m_rawFileInterface, id, p->second.m_page.get());
        m_divertedPageIds[it->m_id] = id;
        m_newPageIds.insert(id);
    }
}

void CacheManager::evictNewPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        assert(it->m_pageClass == PageClass::New);
        auto p = m_pageCache.find(it->m_id);
        assert(p != m_pageCache.end());
        TxFs::writePage(m_rawFileInterface, p->first, p->second.m_page.get());
    }
}

void CacheManager::removeFromCache(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end)
{
    for (auto it = begin; it != end; ++it)
        m_pageCache.erase(it->m_id);
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

CommitHandler CacheManager::buildCommitHandler()
{
    CommitHandler ch(m_rawFileInterface, m_pageCache, std::move(m_divertedPageIds), std::move(m_newPageIds));
    m_divertedPageIds.clear();
    m_newPageIds.clear();
    return ch;
}