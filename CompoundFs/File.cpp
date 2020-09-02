

#include "File.h"
#include "windows.h"

#include <filesystem>
#include <cstdio>

#pragma warning(disable : 4996) // disable "'tmpnam': This function or variable may be unsafe."


using namespace TxFs;

File::File()
    : File(INVALID_HANDLE_VALUE, false)
{}

File::File(File&& other)
    : File(other.m_handle, other.m_readOnly)
{
    other.m_handle = INVALID_HANDLE_VALUE;
}

File::File(void* handle, bool readOnly)
    : m_handle(handle)
    , m_readOnly(readOnly)
    , m_lockProtocol { FileSharedMutex { handle, (uint64_t) -2, (uint64_t) -1 },
                       FileSharedMutex { handle, (uint64_t) 0, (uint64_t) -4 },
                       FileSharedMutex { handle, (uint64_t) -3, (uint64_t) -2 } }
{
}

File& File::operator=(File&& other)
{
    close();
    m_handle = other.m_handle;
    m_readOnly = other.m_readOnly;
    other.m_handle = INVALID_HANDLE_VALUE;
    return *this;
}

File::~File()
{
    close();
}

void File::close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
        ::CloseHandle(m_handle);
    m_handle = INVALID_HANDLE_VALUE;
}

File File::create(std::filesystem::path path) 
{ 
    auto handle = ::CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS,
                 FILE_ATTRIBUTE_NORMAL, nullptr);

    if (handle == INVALID_HANDLE_VALUE)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::create()");

    return File(handle, false);

}


File TxFs::File::createTemp()
{
    auto path = std::filesystem::temp_directory_path() / std::tmpnam(nullptr);
    return create(path);
}


File File::open(std::filesystem::path path, bool readOnly)
{
    auto handle = INVALID_HANDLE_VALUE;
    if (readOnly)
        handle = ::CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    else
        handle = ::CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (handle == INVALID_HANDLE_VALUE)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::open()");

    return File(handle, readOnly);
}

Interval File::newInterval(size_t maxPages)
{
    LARGE_INTEGER size;
    auto succ = ::GetFileSizeEx(m_handle, &size);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::newInterval()");

    succ = ::SetFilePointerEx(m_handle, { (4096 * maxPages) }, nullptr, FILE_END);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::newInterval()");

    succ = ::SetEndOfFile(m_handle);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::newInterval()");

    return Interval(size.QuadPart / 4096, size.QuadPart / 4096 + maxPages);
}

const uint8_t* File::writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    if (pageOffset + (end - begin) > 4096)
        throw std::runtime_error("FileIo::writePage over page boundary");
    if (currentSize() <= id)
        throw std::runtime_error("FileIo::writePage outside file");

    auto succ = ::SetFilePointerEx(m_handle, { (4096 * id) + pageOffset }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::writePage()");

    succ = ::WriteFile(m_handle, begin, end - begin, nullptr, nullptr);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::writePage()");

    return end;
}

const uint8_t* File::writePages(Interval iv, const uint8_t* page)
{
    if (currentSize() < iv.end())
        throw std::runtime_error("FileIo::writePages outside file");

    auto succ = ::SetFilePointerEx(m_handle, { (4096 * iv.begin()) }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::writePages()");
    
    succ = ::WriteFile(m_handle, page, 4096 * iv.length(), nullptr, nullptr);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::writePages()");

    return page + (iv.length() * 4096);
}

uint8_t* File::readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    if (pageOffset + (end - begin) > 4096)
        throw std::runtime_error("FileIo::readPage over page boundary");
    if (currentSize() <= id)
        throw std::runtime_error("FileIo::readPage outside file");

    auto succ = ::SetFilePointerEx(m_handle, { (4096 * id) + pageOffset }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::readPage()");

    succ = ::ReadFile(m_handle, begin, end - begin, nullptr, nullptr);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::readPage()");

    return end;
}

uint8_t* File::readPages(Interval iv, uint8_t* page) const
{
    if (currentSize() < iv.end())
        throw std::runtime_error("FileIo::readPages outside file");

    auto succ = ::SetFilePointerEx(m_handle, { (4096 * iv.begin()) }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::readPages()");

    succ = ::ReadFile(m_handle, page, 4096 * iv.length(), nullptr, nullptr);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::readPages()");

    return page + (iv.length()*4096);
}

size_t File::currentSize() const
{
    LARGE_INTEGER size;
    auto succ = ::GetFileSizeEx(m_handle, &size);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::currentSize()");

    return size.QuadPart / 4096;
}

void File::flushFile()
{
    auto succ = ::FlushFileBuffers(m_handle);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::flushFile()");
}

void File::truncate(size_t numberOfPages)
{
    auto succ = ::SetFilePointerEx(m_handle, { (4096 * numberOfPages) }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::truncate()");

    succ = ::SetEndOfFile(m_handle);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "FileIo::truncate()");
}

Lock File::defaultAccess()
{
    return m_readOnly ? readAccess() : writeAccess();
}

Lock File::readAccess()
{
    throw std::runtime_error("not implmented");
}

Lock File::writeAccess()
{
    throw std::runtime_error("not implmented");
}

CommitLock File::commitAccess(Lock&& writeLock)
{
    throw std::runtime_error("not implmented");
}

std::filesystem::path File::getFileName() const
{
    std::wstring buffer(1028, 0);
    ::GetFinalPathNameByHandle(m_handle, buffer.data(), buffer.size(), VOLUME_NAME_DOS);
    return buffer;
}
