

#include "FileSharedMutex.h"
#include <string>
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

uint64_t rangeLength(std::pair<uint64_t, uint64_t> range)
{
    auto len = range.second - range.first;
    if (!len)
        std::range_error("FileSharedMutex empty range length.");
    return len;
}

}

void FileSharedMutex::lock()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::LockFileEx(m_fileHandle, LOCKFILE_EXCLUSIVE_LOCK, 0, len & 0xffffffff, len >> 32, &ol);

    if (succ == FALSE)
        handleError();
}

bool FileSharedMutex::try_lock()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::LockFileEx(m_fileHandle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, len & 0xffffffff,
                             len >> 32, &ol);
    if (succ == FALSE && ::GetLastError() != ERROR_LOCK_VIOLATION)
        handleError();

    return succ != FALSE;
}

void FileSharedMutex::unlock()
{
    unlockFile();
}

void FileSharedMutex::lock_shared()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::LockFileEx(m_fileHandle, 0, 0, len & 0xffffffff, len >> 32, &ol);

    if (succ == FALSE)
        handleError();
}

bool FileSharedMutex::try_lock_shared()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::LockFileEx(m_fileHandle, LOCKFILE_FAIL_IMMEDIATELY, 0, len & 0xffffffff, len >> 32, &ol);

    if (succ == FALSE && ::GetLastError() != ERROR_LOCK_VIOLATION)
        handleError();

    return succ != FALSE;
}

void FileSharedMutex::unlock_shared()
{
    unlockFile();
}

void FileSharedMutex::unlockFile()
{
    OVERLAPPED ol = toOverLapped(m_lockRange.first);
    auto len = rangeLength(m_lockRange);
    BOOL succ = ::UnlockFileEx(m_fileHandle, 0, len & 0xffffffff, len >> 32, &ol);
    if (succ == FALSE)
        handleError();
}

void FileSharedMutex::handleError()
{
    using namespace std::string_literals;
    auto error = ::GetLastError();
    throw std::runtime_error("FileSharedMutex Win32 API error: "s + std::to_string(error));
}
