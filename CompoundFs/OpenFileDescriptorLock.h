

#pragma once

#include <utility>
#include <stdint.h>

namespace TxFs
{
    ///////////////////////////////////////////////////////////////////////////
    /// Modern file locks for Linux. These are only available after Kernel v3.15. 
    /// Implements the standard std::shared_lock protocol. The posix api is generic enough 
    /// that we only discover non-conformance at run time. Note that OFD locks are
    /// advisory. One can still do any file operations despite the presence of a lock (no-
    /// thing gets enforced). 
    class OpenFileDescriptorLock final
    {
    public:
        OpenFileDescriptorLock(int handle, int64_t begin, int64_t end)
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
        void throwException();
        int lockOperation(short type, bool block);
        void unlockFile();

    private:
        int m_fileHandle;
        std::pair<uint64_t, uint64_t> m_lockRange;
    };
}