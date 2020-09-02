

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
                       FileSharedMutex { handle, (uint64_t) -3, (uint64_t) -2 },
                       FileSharedMutex { handle, (uint64_t) 0, (uint64_t) -3 } }
{
}

File::File(std::filesystem::path path, OpenMode mode)
    : File(open(path, mode), mode == OpenMode::ReadOnly)
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


void* TxFs::File::open(std::filesystem::path path, OpenMode mode)
{
    void* handle = nullptr;
    switch (mode)
    {
    case OpenMode::Create:
        handle = ::CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

    case OpenMode::Open:
        handle = ::CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

    case OpenMode::ReadOnly:
        handle = ::CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

    default:
        throw std::runtime_error("File::open(): unknown OpenMode");
    }

    if (handle == INVALID_HANDLE_VALUE)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::open()");

    return handle;
}

Interval File::newInterval(size_t maxPages)
{
    LARGE_INTEGER size;
    auto succ = ::GetFileSizeEx(m_handle, &size);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::newInterval()");

    succ = ::SetFilePointerEx(m_handle, { (4096 * maxPages) }, nullptr, FILE_END);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::newInterval()");

    succ = ::SetEndOfFile(m_handle);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::newInterval()");

    return Interval(size.QuadPart / 4096, size.QuadPart / 4096 + maxPages);
}

const uint8_t* File::writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    if (pageOffset + (end - begin) > 4096)
        throw std::runtime_error("File::writePage over page boundary");
    if (currentSize() <= id)
        throw std::runtime_error("File::writePage outside file");

    auto succ = ::SetFilePointerEx(m_handle, { (4096 * id) + pageOffset }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::writePage()");

    succ = ::WriteFile(m_handle, begin, end - begin, nullptr, nullptr);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::writePage()");

    return end;
}

const uint8_t* File::writePages(Interval iv, const uint8_t* page)
{
    if (currentSize() < iv.end())
        throw std::runtime_error("File::writePages outside file");

    auto succ = ::SetFilePointerEx(m_handle, { (4096 * iv.begin()) }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::writePages()");
    
    succ = ::WriteFile(m_handle, page, 4096 * iv.length(), nullptr, nullptr);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::writePages()");

    return page + (iv.length() * 4096);
}

uint8_t* File::readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    if (pageOffset + (end - begin) > 4096)
        throw std::runtime_error("File::readPage over page boundary");
    if (currentSize() <= id)
        throw std::runtime_error("File::readPage outside file");

    auto succ = ::SetFilePointerEx(m_handle, { (4096 * id) + pageOffset }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::readPage()");

    succ = ::ReadFile(m_handle, begin, end - begin, nullptr, nullptr);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::readPage()");

    return end;
}

uint8_t* File::readPages(Interval iv, uint8_t* page) const
{
    if (currentSize() < iv.end())
        throw std::runtime_error("File::readPages outside file");

    auto succ = ::SetFilePointerEx(m_handle, { (4096 * iv.begin()) }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::readPages()");

    succ = ::ReadFile(m_handle, page, 4096 * iv.length(), nullptr, nullptr);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::readPages()");

    return page + (iv.length()*4096);
}

size_t File::currentSize() const
{
    LARGE_INTEGER size;
    auto succ = ::GetFileSizeEx(m_handle, &size);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::currentSize()");

    return size.QuadPart / 4096;
}

void File::flushFile()
{
    auto succ = ::FlushFileBuffers(m_handle);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::flushFile()");
}

void File::truncate(size_t numberOfPages)
{
    auto succ = ::SetFilePointerEx(m_handle, { (4096 * numberOfPages) }, nullptr, FILE_BEGIN);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::truncate()");

    succ = ::SetEndOfFile(m_handle);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::truncate()");
}

Lock File::defaultAccess()
{
    return m_readOnly ? readAccess() : writeAccess();
}

Lock File::readAccess()
{
    return m_lockProtocol.readAccess();
}

Lock File::writeAccess()
{
    return m_lockProtocol.writeAccess();
}

CommitLock File::commitAccess(Lock&& writeLock)
{
    return m_lockProtocol.commitAccess(std::move(writeLock));
}

std::filesystem::path File::getFileName() const
{
    std::wstring buffer(1028, 0);
    auto succ = ::GetFinalPathNameByHandle(m_handle, buffer.data(), buffer.size(), VOLUME_NAME_DOS);
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "File::truncate()");

    return buffer;
}

///////////////////////////////////////////////////////////////////////////////

TempFile::TempFile()
    : File(std::filesystem::temp_directory_path() / std::tmpnam(nullptr), OpenMode::Create)
{
    m_path = getFileName();
}

TempFile::TempFile(TempFile&& other)
    : File(std::move(other))
    , m_path(std::move(other.m_path))
{
    other.m_path = std::filesystem::path();
}

TempFile::~TempFile()
{
    close();
    if (!m_path.empty())
        std::filesystem::remove(m_path);
}

TempFile& TempFile::operator=(TempFile&& other)
{
    close();
    if (!m_path.empty())
        std::filesystem::remove(m_path);
    File::operator=(std::move(other));
    m_path = std::move(other.m_path);
    other.m_path = std::filesystem::path();
    return *this;
}
