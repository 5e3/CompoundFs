
#include "CommitHandler.h"
#include "LogPage.h"

using namespace TxFs;

CommitHandler::CommitHandler(RawFileInterface* rfi, PageCache& pageCache, DivertedPageIds&& divertedPageIds,
    NewPageIds&& newPages)
    : m_rawFileInterface(rfi)
    , m_pageCache(pageCache)
    , m_divertedPageIds(std::move(divertedPageIds))
    , m_newPageIds(std::move(newPages))
{

}


/// Get the page-indices for diverted Dirty pages
std::vector<PageIndex> CommitHandler::getDivertedPageIds() const
{
    std::vector<PageIndex> pages;
    pages.reserve(m_divertedPageIds.size());
    for (const auto& [originalPageId, divertedPageId]: m_divertedPageIds)
        pages.push_back(divertedPageId);

    return pages;
}

void CommitHandler::commit()
{
    //// stop allocating from FreeStore
    //m_pageIntervalAllocator = std::function<Interval(size_t)>();

    auto dirtyPageIds = getAllDirtyPageIds();
    {
        // make sure the copies are visible before the Logs
        auto origToCopyPages = copyDirtyPages(dirtyPageIds);
        m_rawFileInterface->flushFile();

        // make sure the Logs are visible before we overwrite original contents
        writeLogs(origToCopyPages);
        m_rawFileInterface->flushFile();
    }

    updateDirtyPages(dirtyPageIds);
    writeCachedPages();
    m_rawFileInterface->flushFile();

    // cut the file
}

/// Get the original ids of the PageClass::Dirty pages. Some of them may
/// still live in m_pageCache the others were probably pushed out by the
/// dirty-page eviction protocol.
std::vector<PageIndex> CommitHandler::getAllDirtyPageIds() const
{
    std::vector<PageIndex> dirtyPageIds;
    dirtyPageIds.reserve(m_divertedPageIds.size());
    for (const auto& [originalPageIdx, divertedPageIdx]: m_divertedPageIds)
        dirtyPageIds.push_back(originalPageIdx);

    for (auto& p: m_pageCache)
        if (p.second.m_pageClass == PageClass::Dirty)
            dirtyPageIds.push_back(p.first);

    return dirtyPageIds;
}

/// Make a copy of the unmodified dirty pages. The original state of these pages
/// is not in the cache so we just copy from the file to a new location in the file.
std::vector<std::pair<PageIndex, PageIndex>> CommitHandler::copyDirtyPages(const std::vector<PageIndex>& dirtyPageIds)
{
    std::vector<std::pair<PageIndex, PageIndex>> origToCopyPages;
    origToCopyPages.reserve(dirtyPageIds.size());

    auto interval = m_rawFileInterface->newInterval(dirtyPageIds.size());
    assert(interval.length() == dirtyPageIds.size()); // here the file is just growing
    auto nextPage = interval.begin();

    for (auto originalPageIdx: dirtyPageIds)
    {
        TxFs::copyPage(m_rawFileInterface, originalPageIdx, nextPage);
        origToCopyPages.emplace_back(originalPageIdx, nextPage++);
    }
    assert(nextPage == interval.end());

    return origToCopyPages;
}

/// Update the original PageClass::DirtyPage pages either from the cache or from the diverted
/// pages and erase them from the cache.
void CommitHandler::updateDirtyPages(const std::vector<PageIndex>& dirtyPageIds)
{
    for (auto origIdx: dirtyPageIds)
    {
        auto id = divertPage(origIdx);
        auto it = m_pageCache.find(id);
        if (it == m_pageCache.end())
        {
            // if the page is not in the cache just physically copy the page from
            // its diverted place. (PageClass::Dirty pages are either in the cache or redirected)
            assert(id != origIdx);
            TxFs::copyPage(m_rawFileInterface, id, origIdx);
        }
        else
        {
            // we have to use the cached page or else we lose updates (if the page is not PageClass::Read)!
            TxFs::writePage(m_rawFileInterface, origIdx, it->second.m_page.get());
            m_pageCache.erase(it);
        }
    }
}

/// Pages that are still in the cache are written to the file.
void CommitHandler::writeCachedPages()
{
    for (const auto& page: m_pageCache)
    {
        assert(page.second.m_pageClass != PageClass::Undefined);
        if (page.second.m_pageClass != PageClass::Read)
            TxFs::writePage(m_rawFileInterface, page.first, page.second.m_page.get());
    }
    m_pageCache.clear();
}

/// Fill the log pages with data and write them to the file.
void CommitHandler::writeLogs(const std::vector<std::pair<PageIndex, PageIndex>>& origToCopyPages)
{
    auto begin = origToCopyPages.begin();
    while (begin != origToCopyPages.end())
    {
        auto pageIndex = m_rawFileInterface->newInterval(1).begin();
        LogPage logPage(pageIndex);
        begin = logPage.pushBack(begin, origToCopyPages.end());
        TxFs::writePage(m_rawFileInterface, pageIndex, &logPage);
    }
}

/// Find the page we moved the original page to or return identity.
PageIndex CommitHandler::divertPage(PageIndex id) const noexcept
{
    auto it = m_divertedPageIds.find(id);
    if (it == m_divertedPageIds.end())
        return id;
    return it->second;
}







