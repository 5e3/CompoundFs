

#pragma once

#include "RawFileInterface.h"
#include "Cache.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace TxFs
{

class CommitHandler
{
public:
    CommitHandler(Cache&& cache) noexcept;
    CommitHandler(CommitHandler&&) = default;
    CommitHandler& operator=(CommitHandler&&) = default;
    
    CommitHandler(const CommitHandler&) = delete;
    CommitHandler& operator=(const CommitHandler&) = delete;

    void commit();
    std::vector<std::pair<PageIndex, PageIndex>> copyDirtyPages(const std::vector<PageIndex>& dirtyPageIds);
    void writeLogs(const std::vector<std::pair<PageIndex, PageIndex>>& origToCopyPages);
    void updateDirtyPages(const std::vector<PageIndex>& dirtyPageIds);
    void writeCachedPages();
    void exclusiveLockedCommit(const std::vector<PageIndex>& dirtyPageIds);

    std::vector<PageIndex> getDivertedPageIds() const;
    std::vector<PageIndex> getDirtyPageIds() const;

private:
    PageIndex divertPage(PageIndex id) const noexcept;

private:
    Cache m_cache;
 
};

}
