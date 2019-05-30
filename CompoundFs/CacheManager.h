

#pragma once

#include "Node.h"
#include "PageAllocator.h"
#include "PageDef.h"
#include "Interval.h"

#include <utility>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace TxFs
{

class RawFileInterface;

///////////////////////////////////////////////////////////////////////////

class CacheManager
{
    struct PageMetaData
    {
        enum { Undefined, Read, New, DirtyRead };

        unsigned int m_type : 2;
        unsigned int m_usageCount : 24;
        unsigned int m_priority : 6;

        constexpr PageMetaData(int type, int priority) noexcept;
    };

    struct CachedPage : PageMetaData
    {
        std::shared_ptr<uint8_t> m_page;

        CachedPage(const std::shared_ptr<uint8_t>& page, int type, int priority = 0);
    };

public:
    struct PageSortItem : PageMetaData
    {
        PageIndex m_id;

        constexpr PageSortItem(const PageMetaData& pmd, PageIndex id) noexcept;
        constexpr PageSortItem(int type, int usageCount, int priority, PageIndex id) noexcept;

        constexpr bool operator<(PageSortItem rhs) const noexcept;
        constexpr bool operator==(PageSortItem rhs) const noexcept;
    };

public:
    CacheManager(RawFileInterface* rfi, uint32_t maxPages = 256) noexcept;
    void setPageIntervalAllocator(const std::function<Interval(size_t)>& pageIntervalAllocator)
    {
        m_pageIntervalAllocator = pageIntervalAllocator;
    }

    PageDef<uint8_t> newPage();
    ConstPageDef<uint8_t> loadPage(PageIndex id);
    PageDef<uint8_t> repurpose(PageIndex index);
    PageDef<uint8_t> makePageWritable(const ConstPageDef<uint8_t>& loadedPage) noexcept;
    void setPageDirty(PageIndex id) noexcept;
    size_t trim(uint32_t maxPages);
    RawFileInterface* getRawFileInterface() const { return m_rawFileInterface; }
    Interval allocatePageInterval(size_t maxPages) noexcept;
    std::vector<PageIndex> getRedirectedPages() const;
    void commit();

private:
    PageIndex newPageIndex() { return allocatePageInterval(1).begin(); }
    PageIndex redirectPage(PageIndex id) const noexcept;
    std::vector<PageSortItem> getUnpinnedPages() const;
    std::vector<std::pair<PageIndex, PageIndex>> copyDirtyPages();
    void writePhysicalLogs(const std::vector<std::pair<PageIndex, PageIndex>>& origToCopyPages);

    void trimCheck() noexcept;
    void evictDirtyPages(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end);
    void evictNewPages(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end);
    void removeFromCache(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end);

private:
    RawFileInterface* m_rawFileInterface;
    std::function<Interval(size_t)> m_pageIntervalAllocator;
    std::unordered_map<PageIndex, CachedPage> m_cache;
    std::unordered_map<PageIndex, PageIndex> m_redirectedPagesMap;
    std::unordered_set<PageIndex> m_newPageSet;
    PageAllocator m_pageAllocator;
    uint32_t m_maxPages;
};

///////////////////////////////////////////////////////////////////////////////

inline constexpr CacheManager::PageMetaData::PageMetaData(int type, int priority) noexcept
    : m_type(type)
    , m_usageCount(0)
    , m_priority(priority)
{}

///////////////////////////////////////////////////////////////////////////////

inline CacheManager::CachedPage::CachedPage(const std::shared_ptr<uint8_t>& page, int type, int priority)
    : PageMetaData(type, priority)
    , m_page(page)
{}

///////////////////////////////////////////////////////////////////////////////

inline constexpr CacheManager::PageSortItem::PageSortItem(const PageMetaData& pmd, PageIndex id) noexcept
    : PageMetaData(pmd)
    , m_id(id)
{}

inline constexpr CacheManager::PageSortItem::PageSortItem(int type, int usageCount, int priority, PageIndex id) noexcept
    : PageMetaData(type, priority)
    , m_id(id)
{
    m_usageCount = usageCount;
}

inline constexpr bool CacheManager::PageSortItem::operator<(const PageSortItem rhs) const noexcept
{
    return std::tie(m_type, m_usageCount, m_priority) > std::tie(rhs.m_type, rhs.m_usageCount, rhs.m_priority);
}

inline constexpr bool CacheManager::PageSortItem::operator==(const PageSortItem rhs) const noexcept
{
    return std::tie(m_type, m_usageCount, m_priority) == std::tie(rhs.m_type, rhs.m_usageCount, rhs.m_priority);
}

}
