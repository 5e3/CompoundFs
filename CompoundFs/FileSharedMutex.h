

#pragma once

#include <utility>
#include <stdint.h>

namespace TxFs
{
    ///////////////////////////////////////////////////////////////////////////
    /// Implements standard win32 file locking with a std::shared_lock interface.
    /// Note that windows file locking is mandatory. Read()/write() operations
    /// will fail if it contradicts the lock.
    class FileLockWindows final
    {
    public:
        FileLockWindows(void* handle, int64_t begin, int64_t end)
            : m_fileHandle(handle)
            , m_lockRange(begin, end)
        {
        }

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