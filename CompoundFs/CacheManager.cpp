
#include "CacheManager.h"
#include "FileInterface.h"
#include "FileIo.h"
#include "TypedCacheManager.h"
#include "LogPage.h"
#include "CommitHandler.h"
#include "RollbackHandler.h"


#include <assert.h>
#include <algorithm>
#include <tuple>
#include <iterator>

using namespace TxFs;

CacheManager::CacheManager(std::unique_ptr<FileInterface> fi, uint32_t maxPages)
    : m_pageMemoryAllocator(maxPages) 
    , m_cache { std::move(fi) }
    , m_maxCachedPages(maxPages)
{
    m_cache.m_lock = m_cache.file()->defaultAccess();
}


/// Delivers a new page. The page is either allocated form the FreeStore or it comes from extending the file. The
/// PageDef<> is writable as it is expected that a new page was requested because you want to write to it.
PageDef<uint8_t> CacheManager::newPage()
{
    auto pageIndex = allocatePageFromFile();
    return asNewPage(pageIndex);
}

/// Treats the page pageIndex as if it is a newly allocated page. The caller has to guarantie that this makes sense.
PageDef<uint8_t> CacheManager::asNewPage(PageIndex pageIndex)
{
    // assert(m_cache.m_pageCache.find(pageIndex) == m_cache.m_pageCache.end());
    auto page = m_pageMemoryAllocator.allocate();
    m_cache.m_pageCache.emplace(pageIndex, CachedPage(page, PageClass::New));
    m_cache.m_newPageIds.insert(pageIndex);
    trimCheck();
    return PageDef<uint8_t>(page, pageIndex);
}

/// Loads the specified page. The page was written by a previous transactions. The return value can be transformed into
/// something writable (makePageWritable()) which in turn makes this page subject to the dirty-page protocol.
ConstPageDef<uint8_t> CacheManager::loadPage(PageIndex origId)
{
    auto id = TxFs::divertPage(m_cache, origId);
    auto it = m_cache.m_pageCache.find(id);
    if (it == m_cache.m_pageCache.end())
    {
        auto page = m_pageMemoryAllocator.allocate();
        TxFs::readSignedPage(m_cache.file(), id, page.get());
        m_cache.m_pageCache.emplace(id, CachedPage(page, PageClass::Read));
        trimCheck();
        return ConstPageDef<uint8_t>(page, origId);
    }

    it->second.m_usageCount++;                               
    return ConstPageDef<uint8_t>(it->second.m_page, origId);
}

/// Reuses a page for new purposes. It works like loadPage() without physically loading the page, followed by
/// setPageDirty(). The page is treated as PageClass::New if we find the page in the m_newPageIds otherwise it will
/// be flagged as PageClass::Dirty. Note: Do not feed regular FreeStore pages to this API (only feed FreeStore
/// MetaData pages) as they unnecessarily end up following the dirty-page protocol.
PageDef<uint8_t> CacheManager::repurpose(PageIndex origId)
{
    auto id = TxFs::divertPage(m_cache, origId);
    PageClass pageClass = m_cache.m_newPageIds.count(id) ? PageClass::New : PageClass::Dirty;
    auto it = m_cache.m_pageCache.find(id);
    if (it == m_cache.m_pageCache.end())
    {
        auto page = m_pageMemoryAllocator.allocate();
        m_cache.m_pageCache.emplace(id, CachedPage(page, pageClass));
        trimCheck();
        return PageDef<uint8_t>(page, origId);
    }

    it->second.m_usageCount++;
    it->second.setPageClass(pageClass);
    return PageDef<uint8_t>(it->second.m_page, origId);
}

/// Marks that a page was changed: Pages previously read-in are marked dirty (which makes them follow the
/// dirty-page protocoll). All other pages are treated as PageClass::New.
void CacheManager::setPageDirty(PageIndex id) noexcept
{
    id = TxFs::divertPage(m_cache, id);
    PageClass pageClass = m_cache.m_newPageIds.count(id) ? PageClass::New : PageClass::Dirty;

    auto it = m_cache.m_pageCache.find(id);
    assert(it != m_cache.m_pageCache.end());
    it->second.setPageClass(pageClass);
}

/// Finds out if a trim operation needs to be performed and does it if necessary.
void CacheManager::trimCheck()
{
    if (m_cache.m_pageCache.size() > m_maxCachedPages)
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
    return m_cache.m_pageCache.size();
}

/// Use installed allocation function or the rawFileInterface.
Interval CacheManager::allocatePageInterval(size_t maxPages)
{
    if (m_pageIntervalAllocator)
    {
        auto iv = m_pageIntervalAllocator(maxPages);
        if (iv.begin() != PageIdx::INVALID)
            return iv;
        m_pageIntervalAllocator = std::function<Interval(size_t)>();
    }
    return m_cache.m_fileInterface->newInterval(maxPages);
}

/// Find all pages that are currently not pinned.
std::vector<PrioritizedPage> CacheManager::getUnpinnedPages() const
{
    std::vector<PrioritizedPage> pageSortItems;
    pageSortItems.reserve(m_cache.m_pageCache.size());
    for (const auto& cp: m_cache.m_pageCache)
        if (cp.second.m_page.use_count() == 1) // we don't use weak_ptr => this is save
            pageSortItems.emplace_back(cp.second, cp.first);
    return pageSortItems;
}

void CacheManager::evictDirtyPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        assert(it->m_pageClass == PageClass::Dirty);
        auto p = m_cache.m_pageCache.find(it->m_id);
        assert(p != m_cache.m_pageCache.end());
        auto id = allocatePageFromFile();
        TxFs::writeSignedPage(m_cache.file(), id, p->second.m_page.get());
        m_cache.m_divertedPageIds[it->m_id] = id;
        m_cache.m_newPageIds.insert(id);
    }
}

void CacheManager::evictNewPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        assert(it->m_pageClass == PageClass::New);
        auto p = m_cache.m_pageCache.find(it->m_id);
        assert(p != m_cache.m_pageCache.end());
        TxFs::writeSignedPage(m_cache.file(), p->first, p->second.m_page.get());
    }
}

void CacheManager::removeFromCache(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end)
{
    for (auto it = begin; it != end; ++it)
        m_cache.m_pageCache.erase(it->m_id);
}

CommitHandler CacheManager::getCommitHandler()
{
    return CommitHandler(m_cache);
}

RollbackHandler CacheManager::getRollbackHandler()
{
    return RollbackHandler(m_cache);
}


// TODO: Needed for testing. Unclear what we do with this?
std::unique_ptr<FileInterface> CacheManager::handOverFile()
{
    m_cache.m_lock.release(); // we have to release that lock
    return std::move(m_cache.m_fileInterface);
}


