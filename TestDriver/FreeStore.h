#pragma once

#include "FileDescriptor.h"
#include "IntervalSequence.h"
#include "PageManager.h"
#include "FileTable.h"
#include <vector>
#include <unordered_set>
#include <assert.h>

namespace CompFs
{

    class FreeStore
    {
    public:
        FreeStore(std::shared_ptr<PageManager> pageManager, FileDescriptor fd)
            : m_pageManager(pageManager)
            , m_fileDescriptor(fd)
            , m_currentFileSize(fd.m_fileSize)
        {
            assert(fd != FileDescriptor());
        }

        Interval allocate(uint32_t maxPages)
        {
            if (m_currentFileSize == 0)
                return Interval();

            if (!m_pinnedPage)
                loadCurrentIntervals();

            auto next = m_pinnedPage->getNext();
            while (next != Node::INVALID_NODE && m_current.totalLength() < maxPages)
            {
                m_freePageTables.insert(next);
                next = loadFileTablePage(next, m_current);
            }
            
            if (next != m_pinnedPage->getNext())
            {
                m_pinnedPage->setNext(next);
                m_current.sort();
            }

            auto iv = m_current.popFront(maxPages);
            m_currentFileSize -= iv.length() * 4096;
            return iv;
        }

        void deallocate(uint32_t page)
        {
            m_freePageTables.insert(page);
        }

        void deleteFile(FileDescriptor fd)
        {
            assert(fd != FileDescriptor());

            // round up to page size
            fd.m_fileSize = ((fd.m_fileSize + 4096 - 1) / 4096) * 4096;
            m_filesToDelete.push_back(fd);
        }

        void compact();
        FileDescriptor close()
        {
            if (m_fileDescriptor.m_fileSize != m_currentFileSize)
            {
                if (m_pinnedPage->getNext() == Node::INVALID_NODE)
                    m_fileDescriptor.m_last = m_fileDescriptor.m_first;
                m_pinnedPage->clear();
                m_pageManager->pageDirty(m_fileDescriptor.m_first);
                m_fileDescriptor.m_fileSize = m_currentFileSize;
            }

            finalize();
            FileDescriptor fd;
            std::swap(fd, m_fileDescriptor);
            m_pageManager.reset();
            m_pinnedPage.reset();
            m_freePageTables.clear();
            m_current.clear();

            return fd;
        }

    private:
        Node::Id loadFileTablePage(Node::Id page, IntervalSequence& is) const
        {
            auto pageTable = m_pageManager->loadFileTable(page).first;
            pageTable->insertInto(is);
            return pageTable->getNext();
        }

        template<typename TIter>
        IntervalSequence loadIntervals(TIter begin, TIter end) const
        {
            IntervalSequence is;
            for (auto it = begin; it != end; ++it)
            {
                Node::Id page = it->m_first;
                while (page != Node::INVALID_NODE)
                    page = loadFileTablePage(page, is);
            }
            return is;
        }

        void loadCurrentIntervals()
        {
            if (!m_pinnedPage)
            {
                m_pinnedPage = m_pageManager->loadFileTable(m_fileDescriptor.m_first).first;
                m_pinnedPage->insertInto(m_current);
            }
        }

        IntervalSequence onePageOptimization()
        {
            // separate one-page FileTable files from the others
            auto pos = std::partition(m_filesToDelete.begin(), m_filesToDelete.end(), [](FileDescriptor fd) { return fd.m_first == fd.m_last; });

            // load all one-page FileTables into IntervalSequence
            auto is = loadIntervals(m_filesToDelete.begin(), pos);

            // move these pages to m_freePageTables
            for (auto it = m_filesToDelete.begin(); it != pos; ++it)
                m_freePageTables.insert(it->m_first);

            // lets just keep the files with longer tables
            m_filesToDelete.erase(m_filesToDelete.begin(), pos);

            // add the freePageTables to the IntervalSequence so we can reuse it
            for (auto page : m_freePageTables)
                is.pushBack(Interval(page));

            // if by now we have anything add m_current intervals to the IntervalSequence
            if (!is.empty())
                loadCurrentIntervals();
            m_current.moveTo(is);

            // good chance that we can make this shorter by sorting
            is.sort();
            return is;
        }

        FileDescriptor pushFileTables(IntervalSequence& is) const
        {
            FileDescriptor fd = m_fileDescriptor;
            if (is.empty())
                return fd;

            PageManager::FileTablePage cur = PageManager::FileTablePage(m_pinnedPage, fd.m_first);
            m_pageManager->pageDirty(cur.second);
            cur.first->transferFrom(is);
            while (!is.empty())
            {
                // let's use one of our pages as a FileTable
                auto pageId = is.popFront(1).begin();
                auto next = m_pageManager->loadFileTable(pageId);  //!! 
                if (m_freePageTables.count(pageId))
                    m_pageManager->pageDirty(pageId);
                next.first->setNext(cur.first->getNext());
                cur.first->setNext(pageId);
                cur = next;
                cur.first->transferFrom(is);
            }

            if (fd.m_first == fd.m_last)
                fd.m_last = cur.second;

            return fd;
        }

        void finalize()
        {
            for (auto& fd : m_filesToDelete)
                m_fileDescriptor.m_fileSize += fd.m_fileSize;

            auto is = onePageOptimization();
            m_fileDescriptor.m_fileSize += m_freePageTables.size() * 4096;

            FileDescriptor cur = pushFileTables(is);
            for (auto& fd : m_filesToDelete)
                cur = chainFiles(cur, fd);
            m_filesToDelete.clear();
            m_fileDescriptor = cur;
        }

        FileDescriptor chainFiles(FileDescriptor prev, FileDescriptor next)
        {
            PageManager::FileTablePage ft = m_pageManager->loadFileTable(prev.m_last);
            assert(ft.first->getNext() == Node::INVALID_NODE);
            ft.first->setNext(next.m_first);
            m_pageManager->pageDirty(prev.m_last);

            assert(prev.m_fileSize % 4096 == 0);
            assert(next.m_fileSize % 4096 == 0);
            prev.m_last = next.m_last;
            return prev;
        }

    private:
        std::shared_ptr<PageManager> m_pageManager;
        FileDescriptor m_fileDescriptor;
        uint64_t m_currentFileSize;
        std::vector<FileDescriptor> m_filesToDelete;
        std::unordered_set<Node::Id> m_freePageTables;
        IntervalSequence m_current;
        std::shared_ptr<FileTable> m_pinnedPage;


    };
}


