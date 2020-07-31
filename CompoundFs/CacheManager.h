

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
/// The PageClass controls the cache-eviction protocol of the CacheManager
/// and defines how pages are persisted during the commit-phase.

enum class PageClass : uint8_t {
    Undefined,
    Read, // page was read
    New,  // previously unused page, always modified
    Dirty // modified, previously used page
};

///////////////////////////////////////////////////////////////////////////
/// All meta-data pages involve the CacheManager. It caches pages implementing
/// a transparent cache-eviction-strategy to ensure an upper bound memory
/// limit is met at all times. During the commit-phase the CacheManager
/// persists enough information to allow rollback from *any* incomplete update
/// operation. As a result a write operation either completes or does not
/// affect the previous state of the file therefore the file is never corrupted.

class CacheManager
{
    struct PageMetaData
    {
        unsigned int m_pageClass : 2;
        unsigned int m_usageCount : 24;
        unsigned int m_priority : 6;

        constexpr PageMetaData(PageClass pageClass, int priority) noexcept;
    };

    struct CachedPage : PageMetaData
    {
        std::shared_ptr<uint8_t> m_page;

        CachedPage(const std::shared_ptr<uint8_t>& page, PageClass pageClass, int priority = 0);
    };

public:
    struct PrioritizedPage : PageMetaData
    {
        PageIndex m_id;

        constexpr PrioritizedPage(const PageMetaData& pmd, PageIndex id) noexcept;
        constexpr PrioritizedPage(PageClass pageClass, int usageCount, int priority, PageIndex id) noexcept;

        constexpr bool operator<(PrioritizedPage rhs) const noexcept;
        constexpr bool operator==(PrioritizedPage rhs) const noexcept;
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
    std::vector<PrioritizedPage> getUnpinnedPages() const;
    std::vector<std::pair<PageIndex, PageIndex>> copyDirtyPages();
    void writeLogs(const std::vector<std::pair<PageIndex, PageIndex>>& origToCopyPages);

    void trimCheck() noexcept;
    void evictDirtyPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end);
    void evictNewPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end);
    void removeFromCache(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end);

private:
    RawFileInterface* m_rawFileInterface;
    std::function<Interval(size_t)> m_pageIntervalAllocator;
    std::unordered_map<PageIndex, CachedPage> m_cache;
    std::unordered_map<PageIndex, PageIndex> m_redirectedPagesMap;
    std::unordered_set<PageIndex> m_newPageSet;
    PageAllocator m_pageMemoryAllocator;
    uint32_t m_maxCachedPages;
};

///////////////////////////////////////////////////////////////////////////////

inline constexpr CacheManager::PageMetaData::PageMetaData(PageClass pageClass, int priority) noexcept
    : m_pageClass(static_cast<unsigned>(pageClass))
    , m_usageCount(0)
    , m_priority(priority)
{}

///////////////////////////////////////////////////////////////////////////////

inline CacheManager::CachedPage::CachedPage(const std::shared_ptr<uint8_t>& page, PageClass pageClass, int priority)
    : PageMetaData(pageClass, priority)
    , m_page(page)
{}

///////////////////////////////////////////////////////////////////////////////

inline constexpr bool operator==(unsigned lhs, PageClass rhs)
{
    return lhs == static_cast<unsigned>(rhs);
}

inline constexpr bool operator==(PageClass lhs, unsigned rhs)
{
    return rhs == lhs;
}

///////////////////////////////////////////////////////////////////////////////

inline constexpr CacheManager::PrioritizedPage::PrioritizedPage(const PageMetaData& pmd, PageIndex id) noexcept
    : PageMetaData(pmd)
    , m_id(id)
{}

inline constexpr CacheManager::PrioritizedPage::PrioritizedPage(PageClass pageClass, int usageCount, int priority,
                                                                PageIndex id) noexcept
    : PageMetaData(pageClass, priority)
    , m_id(id)
{
    m_usageCount = usageCount;
}

inline constexpr bool CacheManager::PrioritizedPage::operator<(const PrioritizedPage rhs) const noexcept
{
    return std::tie(m_pageClass, m_usageCount, m_priority)
           > std::tie(rhs.m_pageClass, rhs.m_usageCount, rhs.m_priority);
}

inline constexpr bool CacheManager::PrioritizedPage::operator==(const PrioritizedPage rhs) const noexcept
{
    return std::tie(m_pageClass, m_usageCount, m_priority)
           == std::tie(rhs.m_pageClass, rhs.m_usageCount, rhs.m_priority);
}

}
