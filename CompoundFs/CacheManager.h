

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
    typedef std::pair<std::shared_ptr<uint8_t>, PageIndex> Page;

    struct PageMetaData
    {
        enum { Undefined, Read, New, DirtyRead };

        unsigned int m_type : 2;
        unsigned int m_usageCount : 24;
        unsigned int m_priority : 6;

        PageMetaData(int type, int priority);
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

        PageSortItem(const PageMetaData& pmd, PageIndex id);
        PageSortItem(int type, int usageCount, int priority, PageIndex id);

        bool operator<(PageSortItem rhs) const;
        bool operator==(PageSortItem rhs) const;
    };

public:
    CacheManager(RawFileInterface* rfi, uint32_t maxPages = 256) noexcept;
    void setPageIntervalAllocator(const std::function<Interval(size_t)>& pageIntervalAllocator)
    {
        m_pageIntervalAllocator = pageIntervalAllocator;
    }

    PageDef<uint8_t> newPage();
    ConstPageDef<uint8_t> loadPage(PageIndex id);
    PageDef<uint8_t> repurpose(PageIndex index, bool forceNew = false);
    PageDef<uint8_t> makePageWritable(const ConstPageDef<uint8_t>& loadedPage) noexcept;
    void setPageDirty(PageIndex id) noexcept;
    size_t trim(uint32_t maxPages);
    RawFileInterface* getRawFileInterface() const { return m_rawFileInterface; }
    Interval allocatePageInterval(size_t maxPages) noexcept;

private:
    PageIndex newPageIndex() { return allocatePageInterval(1).begin(); }
    PageIndex redirectPage(PageIndex id) const noexcept;
    std::vector<PageSortItem> getUnpinnedPages() const;

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

}
