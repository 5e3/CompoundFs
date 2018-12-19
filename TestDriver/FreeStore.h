#pragma once

#include "FileDescriptor.h"
#include "IntervalSequence.h"
#include "PageManager.h"
#include "FileTable.h"
#include <vector>
#include <assert.h>

namespace CompFs
{

    class FreeStore
    {
    public:
        FreeStore(std::shared_ptr<PageManager> pageManager, FileDescriptor fd)
            : m_pageManager(pageManager)
            , m_fileDescriptor(fd)
        {
            assert(fd != FileDescriptor());
        }

        Interval allocate(uint32_t maxPages);
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
            finalize();
            return m_fileDescriptor;
        }

    private:
        template<typename TIter>
         IntervalSequence readIntervals(TIter begin, TIter end) const
         {
             IntervalSequence is;
             for (auto it=begin; it!=end; ++it)
             {
                 Node::Id page = it->m_first;
                 while (page != Node::INVALID_NODE)
                 {
                    auto pageTable = m_pageManager->loadFileTable(page).first;
                    pageTable->insertInto(is);
                    page = pageTable->getNext();
                 }
             }
             return is;
         }

        void addCurrentIntervals(IntervalSequence& is)
        {
            if (!m_pinnedPage)
            {
                m_pinnedPage = m_pageManager->loadFileTable(m_fileDescriptor.m_first).first;
                m_pinnedPage->insertInto(is);
            }
            else
                while(!m_current.empty())
                    is.pushBack(m_current.popFront());
        }

        void finalize()
        {
            auto pos = std::partition(m_filesToDelete.begin(), m_filesToDelete.end(), [](FileDescriptor fd) { return fd.m_first == fd.m_last; });
            auto is = readIntervals(m_filesToDelete.begin(), pos);
            for (auto it= m_filesToDelete.begin(); it!=m_filesToDelete.end(); ++it)
                is.pushBack(Interval(it->m_first, it->m_first+1));
            while (!m_freePageTables.empty())
                is.pushBack(m_freePageTables.popFront());
            if (!is.empty())
                addCurrentIntervals(is);

            is.sort();

            FileDescriptor cur = m_fileDescriptor;
            for (auto& fd: m_filesToDelete)
                cur = chainFiles(cur, fd);
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
            prev.m_fileSize += next.m_fileSize;
            prev.m_last = next.m_last;
            return prev;
        }

    private:
        std::shared_ptr<PageManager> m_pageManager;
        FileDescriptor m_fileDescriptor;
        std::vector<FileDescriptor> m_filesToDelete;
        IntervalSequence m_freePageTables;
        IntervalSequence m_current;
        std::shared_ptr<FileTable> m_pinnedPage;


    };
}


