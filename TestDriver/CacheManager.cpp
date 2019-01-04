
#include "stdafx.h"
#include "CacheManager.h"
#include <assert.h>
#include <algorithm>

using namespace CompFs;

CacheManager::CacheManager(RawFileInterface* rfi, uint32_t maxPages)
    : m_rawFileInterface(rfi)
    , m_pageAllocator(maxPages)
    , m_maxPages(maxPages)
{
}

CacheManager::Page CacheManager::newPage()
{
    auto page = m_pageAllocator.allocate();
    auto id = m_rawFileInterface->newPage();
    m_cache.insert(std::make_pair(id, CachedPage(page, CachedPage::New)));
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
        return page;
    }
    
    it->second.m_usageCount++;
    return it->second.m_page;
}

void CacheManager::pageDirty(Node::Id id)
{
    id = redirectPage(id);
    int type = m_newPageSet.count(id) ? CachedPage::New : CachedPage::DirtyRead;

    auto it = m_cache.find(id);
    assert(it != m_cache.end());
    it->second.m_type = type;
}

size_t CacheManager::trim(uint32_t maxPages)
{
    auto pageSortItems = getUnpinnedPages();
    maxPages = std::min(maxPages, (uint32_t) pageSortItems.size());
    auto beginEvictSet = pageSortItems.begin()+maxPages;

    std::nth_element(pageSortItems.begin(), beginEvictSet, pageSortItems.end());
    auto beginNewPageSet = std::partition(beginEvictSet, pageSortItems.end(), [] (PageSortItem psi) {return psi.m_type == PageMetaData::DirtyRead;});
    auto endNewPageSet = std::partition(beginNewPageSet, pageSortItems.end(), [] (PageSortItem psi) {return psi.m_type == PageMetaData::New;});

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
    for(auto it=begin; it!=end; ++it)
    {
        assert(it->m_type == PageMetaData::DirtyRead);
        auto p = m_cache.find(it->m_id);
        assert(p != m_cache.end());
        auto id = m_rawFileInterface->newPage();
        m_rawFileInterface->writePage(id, p->second.m_page);
        m_redirectedPagesMap[it->m_id] = id;
    }
}

void CacheManager::evictNewPages(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end)
{
    for(auto it=begin; it!=end; ++it)
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
    for(auto it=begin; it!=end; ++it)
        m_cache.erase(it->m_id);
}


