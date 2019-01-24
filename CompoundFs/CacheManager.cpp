
#include "CacheManager.h"
#include <assert.h>
#include <algorithm>

using namespace TxFs;

CacheManager::CacheManager(RawFileInterface* rfi, uint32_t maxPages)
    : m_rawFileInterface(rfi)
    , m_pageAllocator(maxPages)
    , m_maxPages(maxPages)
{}

CacheManager::Page CacheManager::newPage()
{
    auto page = m_pageAllocator.allocate();
    auto id = m_rawFileInterface->newPage();
    m_cache.insert(std::make_pair(id, CachedPage(page, CachedPage::New)));
    trimCheck();
    return Page(page, id);
}

std::shared_ptr<uint8_t> CacheManager::getPage(Node::Id id)
{
    id = redirectPage(id);
    auto it = m_cache.find(id);
    if (it == m_cache.end())
    {
        auto page = m_pageAllocator.allocate();
        m_rawFileInterface->readPage(id, page);
        m_cache.insert(std::make_pair(id, CachedPage(page, CachedPage::Read)));
        trimCheck();
        return page;
    }

    it->second.m_usageCount++;
    return it->second.m_page;
}

void CacheManager::setPageDirty(Node::Id id)
{
    id = redirectPage(id);
    int type = m_newPageSet.count(id) ? CachedPage::New : CachedPage::DirtyRead;

    auto it = m_cache.find(id);
    assert(it != m_cache.end());
    it->second.m_type = type;
}

void CacheManager::trimCheck()
{
    if (m_cache.size() > m_maxPages)
        trim(m_maxPages / 4 * 3);
}

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

Node::Id CacheManager::redirectPage(Node::Id id) const
{
    auto it = m_redirectedPagesMap.find(id);
    if (it == m_redirectedPagesMap.end())
        return id;
    return it->second;
}

std::vector<CacheManager::PageSortItem> CacheManager::getUnpinnedPages() const
{
    std::vector<PageSortItem> pageSortItems;
    pageSortItems.reserve(m_cache.size());
    for (auto& cp: m_cache)
        if (cp.second.m_page.unique())
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
        auto id = m_rawFileInterface->newPage();
        m_rawFileInterface->writePage(id, p->second.m_page);
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
        m_rawFileInterface->writePage(p->first, p->second.m_page);
        m_newPageSet.insert(p->first);
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

CacheManager::PageSortItem::PageSortItem(const PageMetaData& pmd, Node::Id id)
    : PageMetaData(pmd)
    , m_id(id)
{}

CacheManager::PageSortItem::PageSortItem(int type, int usageCount, int priority, Node::Id id)
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
