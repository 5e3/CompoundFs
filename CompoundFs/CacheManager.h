

#pragma once

#include "Node.h"
#include "PageAllocator.h"
#include "PageDef.h"
#include "Interval.h"
#include "PageMetaData.h"
#include "Cache.h"

#include <utility>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace TxFs
{

class RawFileInterface;
class CommitHandler;

///////////////////////////////////////////////////////////////////////////
/// All meta-data pages involve the CacheManager. It caches pages implementing
/// a transparent cache-eviction-strategy to ensure an upper bound memory
/// limit is met at all times. During the commit-phase the CacheManager
/// persists enough information to allow rollback from *any* incomplete update
/// operation. As a result a write operation either completes or does not
/// affect the previous state of the file therefore the file is never corrupted.
class CacheManager
{
public:
    CacheManager(RawFileInterface* rfi, uint32_t maxPages = 256) noexcept;
    CacheManager(CacheManager&&) = default;

    template <typename TCallable>
    void setPageIntervalAllocator(TCallable&&);

    PageDef<uint8_t> newPage();
    ConstPageDef<uint8_t> loadPage(PageIndex id);
    PageDef<uint8_t> repurpose(PageIndex index);
    PageDef<uint8_t> makePageWritable(const ConstPageDef<uint8_t>& loadedPage) noexcept;
    void setPageDirty(PageIndex id) noexcept;
    Interval allocatePageInterval(size_t maxPages) noexcept;
    size_t trim(uint32_t maxPages);

    CommitHandler buildCommitHandler();
    std::vector<std::pair<PageIndex, PageIndex>> readLogs() const;
    RawFileInterface* getRawFileInterface() const { return m_cache.m_rawFileInterface; }

private:
    PageIndex newPageIndex() { return allocatePageInterval(1).begin(); }
    std::vector<PrioritizedPage> getUnpinnedPages() const;

    void trimCheck() noexcept;
    void evictDirtyPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end);
    void evictNewPages(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end);
    void removeFromCache(std::vector<PrioritizedPage>::iterator begin, std::vector<PrioritizedPage>::iterator end);

private:
    PageAllocator m_pageMemoryAllocator;
    Cache m_cache;
    std::function<Interval(size_t)> m_pageIntervalAllocator;
    uint32_t m_maxCachedPages;
};

///////////////////////////////////////////////////////////////////////////////

template <typename TCallable>
inline void CacheManager::setPageIntervalAllocator(TCallable&& pageIntervalAllocator)
{
    m_pageIntervalAllocator = pageIntervalAllocator;
}


}
