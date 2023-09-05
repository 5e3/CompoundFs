#pragma once

#include "FileDescriptor.h"
#include "IntervalSequence.h"
#include "TypedCacheManager.h"
#include "FileTable.h"
#include <vector>
#include <unordered_set>
#include <assert.h>

namespace TxFs
{

///////////////////////////////////////////////////////////////////////////
/// The FreeStore manages space that got 'deleted'. It can act as a page-
/// allocator making sure that before a file grows all free space is used.
/// FreeStore pools pages from previous transactions and avoids
/// recently freed-up space to keep the number of dirty pages low. However 
/// during its commit-phase in close() it will use its own page-pool to
/// satisfy page-allocation requests for meta-data pages.

class FreeStore final
{
public:
    FreeStore(const std::shared_ptr<CacheManager>& cacheManager, FileDescriptor fd)
        : m_cacheManager(cacheManager)
        , m_fileDescriptor(fd)
        , m_currentFileSize(fd.m_fileSize)
    {
        assert(fd != FileDescriptor());
    }

    Interval allocate(uint32_t maxPages)
    {
        if (m_currentFileSize == 0)
            return Interval();

        // delayed loading of the first batch of Intervals
        if (!m_freeListHeadPage.m_page)
            loadInitialIntervals();

        // load as many PageTables necessary to potentially fulfill this request
        auto next = m_freeListHeadPage.m_page->getNext();
        while (next != PageIdx::INVALID && m_current.totalLength() < maxPages)
        {
            m_freeMetaDataPages.insert(next);
            next = loadFileTablePage(next, m_current);
        }

        // if we loaded more than one page then sort it. This potentially improves access patterns as it ensures that we
        // write low-index to high-index and it might merge the intervals.
        if (next != m_freeListHeadPage.m_page->getNext())
        {
            // TODO: make nicer
            m_cacheManager.makePageWritable(m_freeListHeadPage).m_page->setNext(next);
            m_current.sort();
        }

        auto iv = m_current.popFront(maxPages);
        m_currentFileSize -= iv.length() * 4096ULL;
        return iv;
    }

    /// Return meta-data-pages to the FreeStore. These pages will be available in the next transaction.
    void deallocate(uint32_t page) { m_freeMetaDataPages.insert(page); }

    /// Defered file deletion. Upon commit-time when close() is called we will add these files to the FreeStore. The
    /// space of these files will be available in the next transaction.
    void deleteFile(FileDescriptor fd)
    {
        if (fd == FileDescriptor())
            return;

        // round up to page size
        fd.m_fileSize = ((fd.m_fileSize + 4096 - 1) / 4096) * 4096;
        m_filesToDelete.push_back(fd);
    }

    FileDescriptor close()
    {
        // if anything was changed establish consistancy before calling finalize()
        if (m_fileDescriptor.m_fileSize != m_currentFileSize)
        {
            auto pinnedPage = m_cacheManager.makePageWritable(m_freeListHeadPage).m_page;
            if (pinnedPage->getNext() == PageIdx::INVALID)
                m_fileDescriptor.m_last = m_fileDescriptor.m_first;
            pinnedPage->clear();
            m_fileDescriptor.m_fileSize = m_currentFileSize;
        }

        finalize();
        FileDescriptor fd;
        std::swap(fd, m_fileDescriptor);
        // m_freeListHeadPage.reset(); TODO: ?
        m_freeMetaDataPages.clear();
        m_current.clear();

        return fd;
    }

private:
    /// Loads one FileTablePage into an IntervalSequence and returns the next page's index
    PageIndex loadFileTablePage(PageIndex page, IntervalSequence& is) const
    {
        auto pageTable = m_cacheManager.loadPage<FileTable>(page).m_page;
        pageTable->insertInto(is);
        return pageTable->getNext();
    }

    template <typename TIter>
    IntervalSequence loadIntervals(TIter begin, TIter end) const
    {
        IntervalSequence is;
        for (auto it = begin; it != end; ++it)
        {
            PageIndex page = it->m_first;
            while (page != PageIdx::INVALID)
                page = loadFileTablePage(page, is);
        }
        return is;
    }

    void loadInitialIntervals()
    {
        if (!m_freeListHeadPage.m_page)
        {
            m_freeListHeadPage = m_cacheManager.loadPage<FileTable>(m_fileDescriptor.m_first);
            m_freeListHeadPage.m_page->insertInto(m_current);
        }
    }

    /// Loads and merges the Intervals of all files consisting of only one FileTable page as these pages are likely only
    /// sparsely filled.
    IntervalSequence onePageOptimization()
    {
        // separate one-page FileTable files from the others
        auto pos = std::partition(m_filesToDelete.begin(), m_filesToDelete.end(),
                                  [](FileDescriptor fd) { return fd.m_first == fd.m_last; });

        // load all one-page FileTables into IntervalSequence
        auto is = loadIntervals(m_filesToDelete.begin(), pos);

        // move these pages to m_freeMetaDataPages
        for (auto it = m_filesToDelete.begin(); it != pos; ++it)
            m_freeMetaDataPages.insert(it->m_first);

        // lets just keep the files with longer tables
        m_filesToDelete.erase(m_filesToDelete.begin(), pos);

        // TODO: split function here

        // add the freePageTables to the IntervalSequence so we can reuse it
        for (auto page: m_freeMetaDataPages)
            is.pushBack(Interval(page));

        // if by now we have anything add m_current intervals to the IntervalSequence
        if (!is.empty())
            loadInitialIntervals();
        m_current.moveTo(is);

        // good chance that we can make this shorter by sorting
        is.sort();
        return is;
    }

    /// Pushes the IntervalSequence back into FileTable pages. If more than one 
    /// page is needed it will try to reuse its own pages to store the FileTables.
    /// Note: FreeStore::close() happens before the CacheManager commits.
    /// Therefore some CasheManager pages need to deallocate() while
    /// still in-use. FreeStore mustn't reuse these pages for its own meta-
    /// data needs otherwise this could result in corrupted pages!
    FileDescriptor pushFileTables(IntervalSequence& is) const
    {
        if (is.empty())
            return m_fileDescriptor;

        FileDescriptor fd = m_fileDescriptor;

        auto cur = m_cacheManager.makePageWritable(m_freeListHeadPage);
        cur.m_page->transferFrom(is);
        while (!is.empty())
        {
            // let's try to reuse one of our own pages as a FileTable but only if the page is not
            // a meta-data page form m_freeMetaDataPages. Fall back: allocate from m_cacheManager. 
            auto pageId = is.front().begin();
            auto next = m_freeMetaDataPages.count(pageId) ? m_cacheManager.newPage<FileTable>()
                                                          : m_cacheManager.repurpose<FileTable>(is.popFront(1).begin());

            // rewire the next pointers in the singly linked list
            next.m_page->setNext(cur.m_page->getNext());
            cur.m_page->setNext(pageId);

            cur = next;
            cur.m_page->transferFrom(is); // move as much as possible to the page
        }

        // TODO: this is incorrect: cur.m_index could be Idx::INVALID
        if (fd.m_first == fd.m_last)
            fd.m_last = cur.m_index;

        return fd;
    }

    void finalize()
    {
        for (const auto& fd: m_filesToDelete)
            m_fileDescriptor.m_fileSize += fd.m_fileSize;

        auto is = onePageOptimization();
        m_fileDescriptor.m_fileSize += m_freeMetaDataPages.size() * 4096ULL;

        FileDescriptor cur = pushFileTables(is);
        for (const auto& fd: m_filesToDelete)
            cur = chainFiles(cur, fd);
        m_filesToDelete.clear();
        m_fileDescriptor = cur;
    }

    /// Links two files together. Makes the last page of prev dirty.
    FileDescriptor chainFiles(FileDescriptor prev, FileDescriptor next)
    {
        auto ft = m_cacheManager.loadPage<FileTable>(prev.m_last);
        assert(ft.m_page->getNext() == PageIdx::INVALID);
        m_cacheManager.makePageWritable(ft).m_page->setNext(next.m_first);

        assert(prev.m_fileSize % 4096 == 0);
        assert(next.m_fileSize % 4096 == 0);

        prev.m_last = next.m_last;
        return prev;
    }

private:
    mutable TypedCacheManager m_cacheManager;
    FileDescriptor m_fileDescriptor;            // the FreeStore looks like a file
    uint64_t m_currentFileSize;                 // tracks the space left before close()
    std::vector<FileDescriptor> m_filesToDelete;
    std::unordered_set<PageIndex> m_freeMetaDataPages;
    IntervalSequence m_current;
    ConstPageDef<FileTable> m_freeListHeadPage; 
};
}
