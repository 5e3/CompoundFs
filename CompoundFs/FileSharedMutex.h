

#pragma once

#include <utility>
#include <stdint.h>

namespace TxFs
{
    class FileSharedMutex
    {
    public:
        void lock();
        bool try_lock();
        void unlock();

        void lock_shared();
        bool try_lock_shared();
        void unlock_shared();

    private:
        void handleError();
        void unlockFile();

    private:
        void* m_fileHandle;
        std::pair<uint64_t, uint64_t> m_lockRange;
    };
}