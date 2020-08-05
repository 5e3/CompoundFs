

#include "FileSharedMutex.h"
#include <stdexcept>
#include <windows.h>

using namespace TxFs;

namespace
{
    OVERLAPPED toOverLapped(uint64_t value)
    {
        OVERLAPPED ol {};
        ol.Offset = value & 0xffffffff;
        ol.OffsetHigh = value >> 32;
        return ol;
    }
}

void FileSharedMutex::lock()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    BOOL succ = ::LockFileEx(m_fileHandle, LOCKFILE_EXCLUSIVE_LOCK, 0, m_lockRange.second & 0xffffffff,
                             m_lockRange.second >> 32, &ol);

    if (succ == FALSE)
        handleError();
}

bool FileSharedMutex::try_lock()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    BOOL succ = ::LockFileEx(m_fileHandle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0,
                             m_lockRange.second & 0xffffffff,
                             m_lockRange.second >> 32, &ol);
    return succ != FALSE;
}

void FileSharedMutex::unlock()
{
    unlockFile();
}

void FileSharedMutex::lock_shared()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    BOOL succ = ::LockFileEx(m_fileHandle, 0, 0, m_lockRange.second & 0xffffffff, m_lockRange.second >> 32, &ol);

    if (succ == FALSE)
        handleError();
}

bool FileSharedMutex::try_lock_shared()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    BOOL succ = ::LockFileEx(m_fileHandle, LOCKFILE_FAIL_IMMEDIATELY, 0, m_lockRange.second & 0xffffffff,
                             m_lockRange.second >> 32, &ol);

    return succ != FALSE;
}

void FileSharedMutex::unlock_shared()
{
    unlockFile();
}

void FileSharedMutex::unlockFile()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    ::UnlockFileEx(m_fileHandle, 0, m_lockRange.second & 0xffffffff, m_lockRange.second >> 32, &ol);

}

void FileSharedMutex::handleError()
{
    auto error = ::GetLastError();
    throw std::runtime_error("Win32 API error");
}
