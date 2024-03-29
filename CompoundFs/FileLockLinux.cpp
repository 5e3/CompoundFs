
#define _GNU_SOURCE // enable OFD locks on GNU
#define PRIVATE // enable OFD locks on macOS

#include "FileLockLinux.h"
#include <string>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

using namespace TxFs;

namespace
{
uint64_t rangeLength(std::pair<uint64_t, uint64_t> range)
{
    auto len = range.second - range.first;
    if (!len)
        throw std::range_error("FileLockLinux empty range length.");
    return len;
}

}

void FileLockLinux::lock()
{
    int ret = lockOperation(F_WRLCK, true);
    if (ret == -1)
        throwException();
}

bool FileLockLinux::try_lock()
{
    int ret = lockOperation(F_WRLCK, false);
    if (ret != -1)
        return true;
    else if (errno != EAGAIN)
        throwException();
    return false;
}

void FileLockLinux::unlock()
{
    unlockFile();
}

void FileLockLinux::lock_shared()
{
    int ret = lockOperation(F_RDLCK, true);
    if (ret == -1)
        throwException();
}

bool FileLockLinux::try_lock_shared()
{
    int ret = lockOperation(F_RDLCK, false);
    if (ret != -1)
        return true;
    else if (errno != EAGAIN)
        throwException();
    return false;
}

void FileLockLinux::unlock_shared()
{
    unlockFile();
}

void FileLockLinux::unlockFile()
{
    int ret = lockOperation(F_UNLCK, false);
    if (ret == -1)
        throwException();
}

void FileLockLinux::throwException()
{
    throw std::system_error(EDOM, std::system_category());
}

int FileLockLinux::lockOperation(short lockOp, bool block)
{
    struct flock fl {};
    fl.l_type = lockOp;
    fl.l_whence = SEEK_SET;

    static_assert(sizeof(fl.l_start) == sizeof(int64_t));
    fl.l_start = (int64_t) m_lockRange.first;

    static_assert(sizeof(fl.l_len) == sizeof(int64_t));
    fl.l_len = (int64_t) rangeLength(m_lockRange);

    fl.l_pid = 0;

    return ::fcntl(m_fileHandle, block ? F_OFD_SETLKW : F_OFD_SETLK, &fl);
}
