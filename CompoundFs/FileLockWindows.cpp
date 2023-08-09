

#include "FileLockWindows.h"
#include <string>
#include <system_error>
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

uint64_t rangeLength(std::pair<uint64_t, uint64_t> range)
{
    auto len = range.second - range.first;
    if (!len)
        throw std::range_error("FileLockWindows empty range length.");
    return len;
}

}

void FileLockWindows::lock()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::LockFileEx(m_fileHandle, LOCKFILE_EXCLUSIVE_LOCK, 0, len & 0xffffffff, len >> 32, &ol);

    if (succ == FALSE)
        handleError();
}

bool FileLockWindows::try_lock()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::LockFileEx(m_fileHandle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, len & 0xffffffff,
                             len >> 32, &ol);
    if (succ == FALSE && ::GetLastError() != ERROR_LOCK_VIOLATION)
        handleError();

    return succ != FALSE;
}

void FileLockWindows::unlock()
{
    unlockFile();
}

void FileLockWindows::lock_shared()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::LockFileEx(m_fileHandle, 0, 0, len & 0xffffffff, len >> 32, &ol);

    if (succ == FALSE)
        handleError();
}

bool FileLockWindows::try_lock_shared()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::LockFileEx(m_fileHandle, LOCKFILE_FAIL_IMMEDIATELY, 0, len & 0xffffffff, len >> 32, &ol);

    if (succ == FALSE && ::GetLastError() != ERROR_LOCK_VIOLATION)
        handleError();

    return succ != FALSE;
}

void FileLockWindows::unlock_shared()
{
    unlockFile();
}

void FileLockWindows::unlockFile()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::UnlockFileEx(m_fileHandle, 0, len & 0xffffffff, len >> 32, &ol);
    if (succ == FALSE)
        handleError();
}

void FileLockWindows::handleError()
{
    throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileLockWindows");
}
