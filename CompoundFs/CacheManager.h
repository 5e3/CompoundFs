

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

class CommitHandler;
class RollbackHandler;

///////////////////////////////////////////////////////////////////////////
/// All meta-data pages involve the CacheManager. It caches pages implementing
/// a transparent cache-eviction-strategy to ensure an upper bound memory
/// limit is met at all times. During the commit-phase the CacheManager
/// persists enough information to allow rollback from *any* incomplete update
/// operation. As a result a write operation either completes or does not
/// affect the previous state of the file therefore the file is never corrupted.
class CacheManager final
{
public:
    CacheManager(std::unique_ptr<FileInterface> fi, uint32_t maxPages = 256);
    CacheManager(CacheManager&&) = default;

    template <typename TCallable>
    void setPageIntervalAllocator(TCallable&&);

    PageDef<uint8_t> newPage();
    PageDef<uint8_t> asNewPage(PageIndex pageIndex);

    ConstPageDef<uint8_t> loadPage(PageIndex id);
    PageDef<uint8_t> repurpose(PageIndex index);
    template <typename TPage> PageDef<TPage> makePageWritable(const ConstPageDef<TPage>& loadedPage) noexcept;
    Interval allocatePageInterval(size_t maxPages);
    size_t trim(uint32_t maxPages);

    CommitHandler getCommitHandler();
    RollbackHandler getRollbackHandler();
    FileInterface* getFileInterface() { return m_cache.file(); }
    std::unique_ptr<FileInterface> handOverFile();

private:
    void setPageDirty(PageIndex id) noexcept;
    PageIndex allocatePageFromFile() { return allocatePageInterval(1).begin(); }
    std::vector<PrioritizedPage> getUnpinnedPages() const;

    void trimCheck();
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

/// Transforms a const page into a writable page. CacheManager needs to know that pages written by a previous
/// transaction are now about to be changed. Such pages are subject to the dirty-page protocol.
template <typename TPage>
inline PageDef<TPage> CacheManager::makePageWritable(const ConstPageDef<TPage>& loadedPage) noexcept
{
    setPageDirty(loadedPage.m_index);
    return PageDef<TPage>(std::const_pointer_cast<TPage>(loadedPage.m_page), loadedPage.m_index);
}




}
