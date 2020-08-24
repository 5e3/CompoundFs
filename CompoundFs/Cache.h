

#pragma once

#include "Node.h"
#include "PageMetaData.h"
#include "Lock.h"
#include "RawFileInterface.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace TxFs
{

struct Cache final
{
    using PageCache = std::unordered_map<PageIndex, CachedPage>;
    using DivertedPageIds = std::unordered_map<PageIndex, PageIndex>;
    using NewPageIds = std::unordered_set<PageIndex>;

    RawFileInterface* file() { return m_rawFileInterface.get(); }
    const RawFileInterface* file() const { return m_rawFileInterface.get(); }

    std::unique_ptr<RawFileInterface> m_rawFileInterface;
    PageCache m_pageCache;
    DivertedPageIds m_divertedPageIds;
    NewPageIds m_newPageIds;
    Lock m_lock;
};

inline PageIndex divertPage(const Cache& cache, PageIndex id)
{
    auto it = cache.m_divertedPageIds.find(id);
    if (it == cache.m_divertedPageIds.end())
        return id;
    return it->second;
}
}
