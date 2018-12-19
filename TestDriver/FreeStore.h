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
            finalizeDeleteFile();
            return m_fileDescriptor;
        }

    private:
        void finalizeDeleteFile()
        {
            FileDescriptor cur = m_fileDescriptor;
            for (auto it = m_filesToDelete.begin(); it != m_filesToDelete.end(); ++it)
                cur = chainDeletedFiles(cur, *it);
            m_fileDescriptor = cur;
        }

        FileDescriptor chainDeletedFiles(FileDescriptor prev, FileDescriptor next)
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
        IntervalSequence m_pagesForFreeStore;
        IntervalSequence m_current;


    };
}


