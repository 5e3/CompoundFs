

#pragma once

#include "RawFileInterface.h"
#include "PageMetaData.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace TxFs
{

class CommitHandler
{
public:
    using PageCache = std::unordered_map<PageIndex, CachedPage>;
    using DivertedPageIds = std::unordered_map<PageIndex, PageIndex>;
    using NewPageIds = std::unordered_set<PageIndex>;

public:
    CommitHandler(RawFileInterface* rfi, PageCache& pageCache, DivertedPageIds&& divertedPageIds, NewPageIds&& newPages);

    void commit();
    std::vector<std::pair<PageIndex, PageIndex>> copyDirtyPages(const std::vector<PageIndex>& dirtyPageIds);
    void writeLogs(const std::vector<std::pair<PageIndex, PageIndex>>& origToCopyPages);
    void updateDirtyPages(const std::vector<PageIndex>& dirtyPageIds);
    void writeCachedPages();
    std::vector<PageIndex> getDivertedPageIds() const;
    std::vector<PageIndex> getAllDirtyPageIds() const;

private:
    PageIndex divertPage(PageIndex id) const noexcept;


private:
    RawFileInterface* m_rawFileInterface;
    PageCache& m_pageCache;
    DivertedPageIds m_divertedPageIds;
    NewPageIds m_newPageIds;
};
}
