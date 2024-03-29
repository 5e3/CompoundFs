
#include "PosixFile.h"
#include "FileLockPosition.h"
#include "Lock.h"
#include <sys/types.h>
#include <fcntl.h>

#ifndef _WINDOWS
    #include <unistd.h>
#else
    #pragma warning(disable : 4996) // disable "'open': The POSIX name for this item is deprecated."
    #include <io.h>
#endif

using namespace TxFs;

namespace posix
{

template <auto TFunc>
struct WrapOsCall
{
    template <typename... TArgs>
    auto operator()(int fh, TArgs&&... args) const
    {
        if(fh < 0)
            throw std::invalid_argument("PosixFile: invalid file handle");
        return operator()<int, TArgs...>(std::forward<int>(fh), std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    auto operator()(TArgs&&... args) const
    {
        auto ret = TFunc(std::forward<TArgs>(args)...);
        if (ret < 0)
            throw std::system_error(EDOM, std::system_category());
        return ret;
    }
};


constexpr auto open = WrapOsCall<::open>();
constexpr auto write = WrapOsCall<::write>();
constexpr auto read = WrapOsCall<::read>();


#ifndef _WINDOWS

    #define O_BINARY 0
    constexpr auto lseek = WrapOsCall<::lseek>();
    constexpr auto fsync = WrapOsCall<::fsync>();
    constexpr auto ftruncate = WrapOsCall<::ftruncate>();

    int fileHandleToLockHandle(int file) { return file; }

#else
    constexpr auto lseek = WrapOsCall<::_lseeki64>();
    constexpr auto fsync = WrapOsCall<::_commit>();

    int ftruncate(int fd, int64_t size)
    {
        if (fd < 0)
            throw std::invalid_argument("PosixFile: invalid file handle");
        auto ret = ::_chsize_s(fd, size);
        if (ret != 0) // different from WrapOsCall!
            throw std::system_error(EDOM, std::system_category());
        return 0;
    }

    void* fileHandleToLockHandle(int file)
    {
        return (void*) (file < 0 ? intptr_t (-1LL) : ::_get_osfhandle(file));
    }
#endif
}

namespace
{
constexpr uint64_t PageSize = 4096ULL;
constexpr uint32_t BlockSize = 16 * 1024 * 1024;
}


PosixFile::PosixFile()
    : PosixFile(-1, false)
{}

PosixFile::~PosixFile()
{
    close();
}

void PosixFile::close()
{
    if (m_file != -1)
        ::close(m_file); // no error handling
    m_file = -1;
}

PosixFile& PosixFile::operator=(PosixFile&& other) noexcept
{
    close();
    m_file = other.m_file;
    m_readOnly = other.m_readOnly;
    other.m_file = -1;
    return *this;
}

PosixFile::PosixFile(PosixFile&& other) noexcept
    : PosixFile(other.m_file, other.m_readOnly)
{
    other.m_file = -1;
}

PosixFile::PosixFile(std::filesystem::path path, OpenMode mode)
    : PosixFile(open(path, mode), mode == OpenMode::ReadOnly)
{
    if (mode == OpenMode::CreateAlways)
    {   // truncate with commit lock
        auto lock = commitAccess(writeAccess());
        truncate(0);
        lock.release();
    }
}

namespace
{

int mapToPosixFileModes(OpenMode mode)
{
    switch (mode)
    {
    case OpenMode::CreateNew:
        return O_CREAT | O_RDWR | O_EXCL | O_BINARY;
    case OpenMode::CreateAlways: // open without truncate
        return O_CREAT | O_RDWR | O_BINARY;
    case OpenMode::Open:
        return O_CREAT | O_RDWR | O_BINARY;
    case OpenMode::OpenExisting:
        return O_RDWR | O_BINARY;
    case OpenMode::ReadOnly:
        return O_RDONLY | O_BINARY;
    default:
        throw std::runtime_error("Unknown OpenMode");
    }
}

}

int PosixFile::open(std::filesystem::path path, OpenMode mode)
{
    return posix::open(path.string().c_str(), mapToPosixFileModes(mode), 0666);
}

PosixFile::PosixFile(int file, bool readOnly)
    : m_file(file)
    , m_readOnly(readOnly)
    , m_lockProtocol{ 
        FileLock { posix::fileHandleToLockHandle(file), FileLockPosition::GateBegin, FileLockPosition::GateEnd },
        FileLock { posix::fileHandleToLockHandle(file), FileLockPosition::SharedBegin, FileLockPosition::SharedEnd },
        FileLock { posix::fileHandleToLockHandle(file), FileLockPosition::WriteBegin, FileLockPosition::WriteEnd },
    }
{
    static constexpr int64_t MaxFileSize = 4096LL * int64_t(std::numeric_limits<uint32_t>::max() - 1LL);
    static_assert(MaxFileSize < FileLockPosition::GateBegin);
    static_assert(FileLockPosition::GateBegin < FileLockPosition::GateEnd);
    static_assert(MaxFileSize < FileLockPosition::SharedBegin);
    static_assert(FileLockPosition::SharedBegin < FileLockPosition::SharedEnd);
    static_assert(MaxFileSize < FileLockPosition::WriteBegin);
    static_assert(FileLockPosition::WriteBegin < FileLockPosition::WriteEnd);
}

Interval PosixFile::newInterval(size_t maxPages)
{
    auto length = posix::lseek(m_file, 0, SEEK_END);
    posix::ftruncate(m_file, length + maxPages * PageSize);
    return Interval(PageIndex(length / PageSize), PageIndex(length / PageSize + maxPages));
}

const uint8_t* PosixFile::writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    if (pageOffset + (end - begin) > PageSize)
        throw std::runtime_error("File::writePage over page boundary");
    if (fileSizeInPages() <= id)
        throw std::runtime_error("File::writePage outside file");

    posix::lseek(m_file, id * PageSize + pageOffset, SEEK_SET);
    posix::write(m_file, begin, unsigned(end - begin));
    return end;
}

const uint8_t* PosixFile::writePages(Interval iv, const uint8_t* page)
{
    if (fileSizeInPages() < iv.end())
        throw std::runtime_error("File::writePages outside file");

    posix::lseek(m_file, iv.begin() * PageSize, SEEK_SET);
    auto end = page + (iv.length() * PageSize);
    writePagesInBlocks(page, end);

    return end;
}

void PosixFile::writePagesInBlocks(const uint8_t* begin, const uint8_t* end)
{
    for (; (begin + BlockSize) < end; begin += BlockSize)
        posix::write(m_file, begin, BlockSize);

    posix::write(m_file, begin, unsigned(end-begin));
}

uint8_t* PosixFile::readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    if (pageOffset + (end - begin) > PageSize)
        throw std::runtime_error("File::readPage over page boundary");
    if (fileSizeInPages() <= id)
        throw std::runtime_error("File::readPage outside file");

    posix::lseek(m_file, id * PageSize + pageOffset, SEEK_SET);
    int bytesRead = posix::read(m_file, begin, unsigned(end - begin));
    return begin + bytesRead;
}

uint8_t* PosixFile::readPages(Interval iv, uint8_t* page) const
{
    if (fileSizeInPages() < iv.end())
        throw std::runtime_error("File::readPages outside file");

    posix::lseek(m_file, iv.begin() * PageSize, SEEK_SET);
    auto end = page + (iv.length() * PageSize);
    size_t bytesRead = readPagesInBlocks(page, end);

    return page + bytesRead;
}

size_t PosixFile::readPagesInBlocks(uint8_t* begin, uint8_t* end) const
{
    size_t bytesRead = 0;
    for (; (begin + BlockSize) < end; begin += BlockSize)
        bytesRead += posix::read(m_file, begin, BlockSize);

    bytesRead += posix::read(m_file, begin, unsigned(end - begin));
    return bytesRead;
}

size_t PosixFile::fileSizeInPages() const
{
    auto bytes = posix::lseek(m_file, 0, SEEK_END);
    bytes += PageSize - 1;
    return bytes / PageSize; // pages rounded up
}

void PosixFile::flushFile()
{
    posix::fsync(m_file);
}

void PosixFile::truncate(size_t numberOfPages)
{ 
    posix::ftruncate(m_file, numberOfPages * PageSize);
}

Lock PosixFile::defaultAccess()
{
    return m_readOnly ? readAccess() : writeAccess();
}

Lock PosixFile::readAccess()
{
    return m_lockProtocol.readAccess();
}

Lock PosixFile::writeAccess()
{
    return m_lockProtocol.writeAccess();
}

CommitLock PosixFile::commitAccess(Lock&& writeLock)
{
    return m_lockProtocol.commitAccess(std::move(writeLock));
}

