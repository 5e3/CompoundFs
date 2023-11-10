

#pragma once

#include "FileInterface.h"
#include "Cache.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace TxFs
{
class CommitLock;

class CommitHandler final
{
public:
    CommitHandler(Cache& cache) noexcept;

    void commit();
    std::vector<std::pair<PageIndex, PageIndex>> copyDirtyPages(const std::vector<PageIndex>& dirtyPageIds);
    void writeLogs(const std::vector<std::pair<PageIndex, PageIndex>>& origToCopyPages);
    void updateDirtyPages(const std::vector<PageIndex>& dirtyPageIds);
    void writeCachedPages();
    CommitLock exclusiveLockedCommit(const std::vector<PageIndex>& dirtyPageIds);
    void lockedWriteCachedPages();

    std::vector<PageIndex> getDivertedPageIds() const;
    std::vector<PageIndex> getDirtyPageIds() const;
    bool empty() const;
    size_t getCompositeSize() const;

private:
    Cache& m_cache;
 
};

}
