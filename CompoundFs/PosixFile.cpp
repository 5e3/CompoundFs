
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
constexpr auto lseek = wrapOsCall(::lseek);




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
        m_file = posix::open(path.string().c_str(), O_CREAT | O_RDWR | O_TRUNC);
        break;

    case OpenMode::Open:
        m_file = posix::open(path.string().c_str(), O_CREAT | O_RDWR);
        break;

    case OpenMode::ReadOnly:
        m_file = posix::open(path.string().c_str(), O_RDONLY);
        break;

    default:
        throw std::runtime_error("File::open(): unknown OpenMode");
    }
}


Interval PosixFile::newInterval(size_t maxPages)
{
    auto length = posix::lseek(m_file, 0, SEEK_END);
    throw std::logic_error("The method or operation is not implemented.");
}

const uint8_t* PosixFile::writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    throw std::logic_error("The method or operation is not implemented.");
}

const uint8_t* PosixFile::writePages(Interval iv, const uint8_t* page)
{
    throw std::logic_error("The method or operation is not implemented.");
}

uint8_t* PosixFile::readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    throw std::logic_error("The method or operation is not implemented.");
}

uint8_t* PosixFile::readPages(Interval iv, uint8_t* page) const
{
    throw std::logic_error("The method or operation is not implemented.");
}

size_t PosixFile::currentSize() const
{
    throw std::logic_error("The method or operation is not implemented.");
}

void PosixFile::flushFile()
{
    throw std::logic_error("The method or operation is not implemented.");
}

void PosixFile::truncate(size_t numberOfPages)
{
    throw std::logic_error("The method or operation is not implemented.");
}

Lock PosixFile::defaultAccess()
{
    throw std::logic_error("The method or operation is not implemented.");
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

