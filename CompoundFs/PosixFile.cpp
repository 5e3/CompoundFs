
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
    if (ret < 0)
        throw std::system_error(EDOM, std::system_category());
}

int open(const char* fname, int mode)
{
    int handle = ::open(fname, mode, 0666);
    throwOnError(handle);
    return handle;
}

template <typename TRet, typename... TArgs>
constexpr auto wrapOsCall(TRet (func)(TArgs...))
{
    return [func](TArgs... args) -> TRet {
        auto returnValue = func(args...);
        throwOnError(returnValue);
        return returnValue;
    };
}

//constexpr auto open = wrapOsCall(::open);
constexpr auto close = wrapOsCall(::close);
constexpr auto write = wrapOsCall(::write);
constexpr auto read = wrapOsCall(::read);

#ifndef _WINDOWS
#define O_BINARY 0;
constexpr auto lseek = wrapOsCall(::lseek);
constexpr auto ftruncate = wrapOsCall(::ftruncate);
constexpr auto fsync = wrapOsCall(::fsync);
#else
constexpr auto lseek = wrapOsCall(::_lseeki64);
constexpr auto ftruncate = wrapOsCall(::_chsize_s);
constexpr auto fsync = wrapOsCall(::_commit);
#endif
}



PosixFile::PosixFile()
{}

PosixFile::~PosixFile()
{
    close();
}

void PosixFile::close()
{
    posix::close(m_file);
}

PosixFile& PosixFile::operator=(PosixFile&&)
{
    throw std::logic_error("The method or operation is not implemented.");
}

PosixFile::PosixFile(PosixFile&&)
{}

PosixFile::PosixFile(std::filesystem::path path, OpenMode mode)
{
    switch (mode)
    {
    case OpenMode::Create:
        m_file = posix::open(path.string().c_str(), O_CREAT | O_RDWR | O_TRUNC | O_BINARY);
        break;

    case OpenMode::Open:
        m_file = posix::open(path.string().c_str(), O_CREAT | O_RDWR | O_BINARY);
        break;

    case OpenMode::ReadOnly:
        m_file = posix::open(path.string().c_str(), O_RDONLY | O_BINARY);
        break;

    default:
        throw std::runtime_error("File::open(): unknown OpenMode");
    }
}


Interval PosixFile::newInterval(size_t maxPages)
{
    auto length = posix::lseek(m_file, 0, SEEK_END);
    posix::ftruncate(m_file, length + maxPages * 4096);
    return Interval(length / 4096, length / 4096 + maxPages);
}

const uint8_t* PosixFile::writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    posix::lseek(m_file, id * 4096 + pageOffset, SEEK_SET);
    posix::write(m_file, begin, end - begin);
    return end;
}

const uint8_t* PosixFile::writePages(Interval iv, const uint8_t* page)
{
    posix::lseek(m_file, iv.begin() * 4096, SEEK_SET);
    posix::write(m_file, page, iv.length()*4096);
    return page + iv.length() * 4096;
}

uint8_t* PosixFile::readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    posix::lseek(m_file, id * 4096 + pageOffset, SEEK_SET);
    posix::read(m_file, begin, end - begin);
    return end;
}

uint8_t* PosixFile::readPages(Interval iv, uint8_t* page) const
{
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

