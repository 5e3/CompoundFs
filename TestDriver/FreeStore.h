#pragma once

#include "FileDescriptor.h"
#include "IntervalSequence.h"
#include <vector>
#include <assert.h>

namespace CompFs
{

    class FreeStore
    {
    public:
        FreeStore(FileDescriptor fd);

        Interval allocate(uint32_t maxPages);
        void deleteFile(FileDescriptor fd)
        {
          assert(fd != FileDescriptor);
          m_filesToDelete.push_back(fd);
        }

        void compact();
        FileDescriptor close();

    private:
      void finalizeDeleteFile();

    private:
      FileDescriptor m_fileDescriptor;
      std::vector<FileDescriptor> m_filesToDelete;
      IntervalSequence m_pagesForFreeStore;
      IntervalSequence m_current;


    };
}


