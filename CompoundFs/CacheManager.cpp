
#include "CacheManager.h"
#include <assert.h>
#include <algorithm>

using namespace TxFs;

CacheManager::CacheManager(RawFileInterface* rfi, uint32_t maxPages)
    : m_rawFileInterface(rfi)
    , m_newPageIndex([rfi]() { return PageIdx::INVALID; })
    , m_pageAllocator(maxPages)
    , m_maxPages(maxPages)
{}

/// Delivers a new page. The page is either allocated form the FreeStore or it comes from extending the file. The
/// PageDef<> is writable as it is expected that a new page was requested because you want to write to it.
PageDef<uint8_t> CacheManager::newPage()
{
    auto page = m_pageAllocator.allocate();
    auto id = newPageIndex();
    m_cache.insert(std::make_pair(id, CachedPage(page, PageMetaData::New)));
    m_newPageSet.insert(id);
    trimCheck();
    return PageDef<uint8_t>(page, id);
}

/// Loads the specified page. The page is loaded because previous transactions left state that is now being accessed.
/// The returnvalue can be transformed into something writable (makePageWritable()) which in turn makes this page
/// subject to the DirtyPage protocoll.
ConstPageDef<uint8_t> CacheManager::loadPage(PageIndex origId)
{
    auto id = redirectPage(origId);
    auto it = m_cache.find(id);
    if (it == m_cache.end())
    {
        auto page = m_pageAllocator.allocate();
        m_rawFileInterface->readPage(id, page.get());
        m_cache.insert(std::make_pair(id, CachedPage(page, PageMetaData::Read)));
        trimCheck();
        return ConstPageDef<uint8_t>(page, origId);
    }

    it->second.m_usageCount++;
    return ConstPageDef<uint8_t>(it->second.m_page, origId);
}

/// Reuses a page for new purposes. This is a dangerous API as it will treat pages
/// never seen before by the CacheManager as PageMetaData::New.
PageDef<uint8_t> CacheManager::repurpose(PageIndex origId)
{
    auto id = redirectPage(origId);
    auto it = m_cache.find(id);
    if (it == m_cache.end())
    {
        auto page = m_pageAllocator.allocate();
        int type = m_redirectedPagesMap.count(id) ? PageMetaData::DirtyRead : PageMetaData::New;
        m_cache.insert(std::make_pair(id, CachedPage(page, type)));
        trimCheck();
        return PageDef<uint8_t>(page, origId);
    }

    it->second.m_usageCount++;
    return PageDef<uint8_t>(it->second.m_page, origId);
}

/// Transforms a const page into a writable page. CacheManager needs to know that pages written by a previous
/// transaction are now about to be changed. Such pages are subject to the DirtyPages protocoll.
PageDef<uint8_t> CacheManager::makePageWritable(const ConstPageDef<uint8_t>& loadedPage)
{
    setPageDirty(loadedPage.m_index);
    return PageDef<uint8_t>(std::const_pointer_cast<uint8_t>(loadedPage.m_page), loadedPage.m_index);
}

/// Marks that a page was changed: Pages previously read-in are marked dirty (which makes them follow the
/// Dirty-Page-Protocoll). All other pages are treated as PageMetaData::New.
void CacheManager::setPageDirty(PageIndex id)
{
    id = redirectPage(id);
    int type = m_newPageSet.count(id) ? PageMetaData::New : PageMetaData::DirtyRead;

    auto it = m_cache.find(id);
    assert(it != m_cache.end());
    it->second.m_type = type;
}

/// Finds out if a trim operation needs to be performed and does it if neccessary.
void CacheManager::trimCheck()
{
    if (m_cache.size() > m_maxPages)
        trim(m_maxPages / 4 * 3);
}

// Trims down memory usage by maxPages. If users have a lot of pinned pages this is triggered too often. Make sure that
// there is sufficient space to deal with real-world scenarious.
size_t CacheManager::trim(uint32_t maxPages)
{
    auto pageSortItems = getUnpinnedPages();
    maxPages = std::min(maxPages, (uint32_t) pageSortItems.size());
    auto beginEvictSet = pageSortItems.begin() + maxPages;

    std::nth_element(pageSortItems.begin(), beginEvictSet, pageSortItems.end());
    auto beginNewPageSet = std::partition(beginEvictSet, pageSortItems.end(),
                                          [](PageSortItem psi) { return psi.m_type == PageMetaData::DirtyRead; });
    auto endNewPageSet = std::partition(beginNewPageSet, pageSortItems.end(),
                                        [](PageSortItem psi) { return psi.m_type == PageMetaData::New; });

    evictDirtyPages(beginEvictSet, beginNewPageSet);
    evictNewPages(beginNewPageSet, endNewPageSet);
    removeFromCache(beginEvictSet, pageSortItems.end());
    return m_cache.size();
}

/// Use installed allocation function or the rawFileInterface.
PageIndex CacheManager::newPageIndex()
{
    auto idx = m_newPageIndex();
    if (idx == PageIdx::INVALID)
    {
        m_newPageIndex = std::function<PageIndex()>([this]() { return this->m_rawFileInterface->newPage(); });
        return m_newPageIndex();
    }

    return idx;
}

/// Find the page we moved the original page to or return identity.
PageIndex CacheManager::redirectPage(PageIndex id) const
{
    auto it = m_redirectedPagesMap.find(id);
    if (it == m_redirectedPagesMap.end())
        return id;
    return it->second;
}

/// Find all pages that are currently not pinned.
std::vector<CacheManager::PageSortItem> CacheManager::getUnpinnedPages() const
{
    std::vector<PageSortItem> pageSortItems;
    pageSortItems.reserve(m_cache.size());
    for (auto& cp: m_cache)
        if (cp.second.m_page.use_count() == 1) // we don't use weak_ptr => this is save
            pageSortItems.push_back(PageSortItem(cp.second, cp.first));
    return pageSortItems;
}

void CacheManager::evictDirtyPages(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        assert(it->m_type == PageMetaData::DirtyRead);
        auto p = m_cache.find(it->m_id);
        assert(p != m_cache.end());
        auto id = newPageIndex();
        m_rawFileInterface->writePage(id, p->second.m_page.get());
        m_redirectedPagesMap[it->m_id] = id;
        m_newPageSet.insert(id);
    }
}

void CacheManager::evictNewPages(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end)
{
    for (auto it = begin; it != end; ++it)
    {
        assert(it->m_type == PageMetaData::New);
        auto p = m_cache.find(it->m_id);
        assert(p != m_cache.end());
        m_rawFileInterface->writePage(p->first, p->second.m_page.get());
    }
}

void CacheManager::removeFromCache(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end)
{
    for (auto it = begin; it != end; ++it)
        m_cache.erase(it->m_id);
}

///////////////////////////////////////////////////////////////////////////////

CacheManager::PageMetaData::PageMetaData(int type, int priority)
    : m_type(type)
    , m_usageCount(0)
    , m_priority(priority)
{}

///////////////////////////////////////////////////////////////////////////////

CacheManager::CachedPage::CachedPage(const std::shared_ptr<uint8_t>& page, int type, int priority)
    : PageMetaData(type, priority)
    , m_page(page)
{}

///////////////////////////////////////////////////////////////////////////////

CacheManager::PageSortItem::PageSortItem(const PageMetaData& pmd, PageIndex id)
    : PageMetaData(pmd)
    , m_id(id)
{}

CacheManager::PageSortItem::PageSortItem(int type, int usageCount, int priority, PageIndex id)
    : PageMetaData(type, priority)
    , m_id(id)
{
    m_usageCount = usageCount;
}

bool CacheManager::PageSortItem::operator<(PageSortItem rhs) const
{
    if (m_type != rhs.m_type)
        return m_type > rhs.m_type;
    if (m_usageCount != rhs.m_usageCount)
        return m_usageCount > rhs.m_usageCount;
    return m_priority > rhs.m_priority;
}

bool CacheManager::PageSortItem::operator==(PageSortItem rhs) const
{
    return m_type == rhs.m_type && m_usageCount == rhs.m_usageCount && m_priority == rhs.m_priority && m_id == rhs.m_id;
}
