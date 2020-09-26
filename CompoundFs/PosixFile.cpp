
#include <sys/types.h>
#include "PosixFile.h"
#include "Lock.h"
#include <fcntl.h>

#ifndef _WINDOWS
#include <unistd.h>
#else
#include <io.h>
#endif

using namespace TxFs;

namespace posix
{

template <typename TInt>
void throwOnError(TInt ret)
{
    if (ret == -1)
        throw std::system_error(EDOM, std::system_category());
}

int open(const char* fname, int mode)
{
    int handle = ::open(fname, mode, 0666);
    if (handle == -1)
        throw std::system_error(EDOM, std::system_category());
    return handle;
}

template <typename TRet, typename... TArgs>
constexpr auto wrapOsCall(TRet (func)(int, TArgs...))
{
    return [func](int file, TArgs... args) -> TRet {
        if (file < 0)
            throw std::invalid_argument("PosixFile: invalid file handle");

        auto returnValue = func(file, args...);
        throwOnError(returnValue);
        return returnValue;
    };
}

constexpr auto write = wrapOsCall(::write);
constexpr auto read = wrapOsCall(::read);

#ifndef _WINDOWS
#define O_BINARY 0
constexpr auto lseek = wrapOsCall(::lseek);
constexpr auto ftruncate = wrapOsCall(::ftruncate);
constexpr auto fsync = wrapOsCall(::fsync);
#else
constexpr auto lseek = wrapOsCall(::_lseeki64);
//constexpr auto ftruncate = wrapOsCall(::_chsize_s);
constexpr auto fsync = wrapOsCall(::_commit);

int ftruncate(int fd, int64_t size)
{
    if (fd < 0)
        throw std::invalid_argument("PosixFile: invalid file handle");
    auto ret = ::_chsize_s(fd, size);
    if (ret != 0)
        throw std::system_error(EDOM, std::system_category());
}
#endif
}

namespace
{
constexpr uint64_t PageSize = 4096ULL;
constexpr int64_t MaxEndOfFile = 0LL;
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

PosixFile& PosixFile::operator=(PosixFile&& other)
{
    close();
    m_file = other.m_file;
    m_readOnly = other.m_readOnly;
    other.m_file = -1;
    return *this;
}

PosixFile::PosixFile(PosixFile&& other)
    : PosixFile(other.m_file, other.m_readOnly)
{
    other.m_file = -1;
}

PosixFile::PosixFile(std::filesystem::path path, OpenMode mode)
    : PosixFile(open(path, mode), mode == OpenMode::ReadOnly)
{
}

int PosixFile::open(std::filesystem::path path, OpenMode mode)
{
    int file = -1;
    switch (mode)
    {
    case OpenMode::Create:
        file = posix::open(path.string().c_str(), O_CREAT | O_RDWR | O_TRUNC | O_BINARY);
        break;

    case OpenMode::Open:
        file = posix::open(path.string().c_str(), O_CREAT | O_RDWR | O_BINARY);
        break;

    case OpenMode::ReadOnly:
        file = posix::open(path.string().c_str(), O_RDONLY | O_BINARY);
        break;

    default:
        throw std::runtime_error("File::open(): unknown OpenMode");
    }

    return file;
}

PosixFile::PosixFile(int file, bool readOnly)
    : m_file(file)
    , m_readOnly(readOnly)
{
}

Interval PosixFile::newInterval(size_t maxPages)
{
    auto length = posix::lseek(m_file, 0, SEEK_END);
    posix::ftruncate(m_file, length + maxPages * 4096);
    return Interval(length / 4096, length / 4096 + maxPages);
}

const uint8_t* PosixFile::writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    if (pageOffset + (end - begin) > PageSize)
        throw std::runtime_error("File::writePage over page boundary");
    if (currentSize() <= id)
        throw std::runtime_error("File::writePage outside file");

    posix::lseek(m_file, id * 4096 + pageOffset, SEEK_SET);
    posix::write(m_file, begin, end - begin);
    return end;
}

const uint8_t* PosixFile::writePages(Interval iv, const uint8_t* page)
{
    if (currentSize() < iv.end())
        throw std::runtime_error("File::writePages outside file");

    posix::lseek(m_file, iv.begin() * 4096, SEEK_SET);
    posix::write(m_file, page, iv.length()*4096);
    return page + iv.length() * 4096;
}

uint8_t* PosixFile::readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    if (pageOffset + (end - begin) > PageSize)
        throw std::runtime_error("File::readPage over page boundary");
    if (currentSize() <= id)
        throw std::runtime_error("File::readPage outside file");

    posix::lseek(m_file, id * 4096 + pageOffset, SEEK_SET);
    posix::read(m_file, begin, end - begin);
    return end;
}

uint8_t* PosixFile::readPages(Interval iv, uint8_t* page) const
{
    if (currentSize() < iv.end())
        throw std::runtime_error("File::readPages outside file");

    posix::lseek(m_file, iv.begin() * 4096, SEEK_SET);
    posix::read(m_file, page, iv.length() * 4096);
    return page + iv.length() * 4096;
}

size_t PosixFile::currentSize() const
{
    return posix::lseek(m_file, 0, SEEK_END) / 4096;
}

void PosixFile::flushFile()
{
    posix::fsync(m_file);
}

void PosixFile::truncate(size_t numberOfPages)
{ 
    posix::ftruncate(m_file, numberOfPages * 4096);
}

Lock PosixFile::defaultAccess()
{
    return writeAccess();
}

Lock PosixFile::readAccess()
{
    throw std::logic_error("The method or operation is not implemented.");
}

Lock PosixFile::writeAccess()
{
    throw std::logic_error("The method or operation is not implemented.");
}

CommitLock PosixFile::commitAccess(Lock&& writeLock)
{
    throw std::logic_error("The method or operation is not implemented.");
}

