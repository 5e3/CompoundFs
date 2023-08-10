

#include "WindowsFile.h"
#include "windows.h"

#include <utility>
#include <filesystem>
#include <cstdio>

#pragma warning(disable : 4996) // disable "'tmpnam': This function or variable may be unsafe."

using namespace TxFs;

namespace Win32
{
void throwOnError(BOOL succ)
{
    if (!succ)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "WindowsFile::OS_Call");
}

void throwOnError(HANDLE handle)
{
    if (handle == INVALID_HANDLE_VALUE)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "WindowsFile::OS_Call");
}

template <typename TRet, typename... TArgs>
constexpr auto wrapOsCall(TRet(__stdcall func)(TArgs...))
{
    return [func](TArgs... args) -> TRet {
        auto returnValue = func(args...);
        throwOnError(returnValue);
        return returnValue;
    };
}

constexpr auto GetFileSizeEx = wrapOsCall(::GetFileSizeEx);
constexpr auto CreateFile = wrapOsCall(::CreateFile);
constexpr auto SetFilePointerEx = wrapOsCall(::SetFilePointerEx);
constexpr auto SetEndOfFile = wrapOsCall(::SetEndOfFile);
constexpr auto WriteFile = wrapOsCall(::WriteFile);
constexpr auto ReadFile = wrapOsCall(::ReadFile);
constexpr auto FlushFileBuffers = wrapOsCall(::FlushFileBuffers);
constexpr auto GetFinalPathNameByHandle = wrapOsCall(::GetFinalPathNameByHandle);

void Seek(HANDLE handle, uint64_t position)
{
    LARGE_INTEGER pos;
    pos.QuadPart = position;
    SetFilePointerEx(handle, pos, nullptr, FILE_BEGIN);
}
}

namespace
{
constexpr uint64_t PageSize = 4096ULL;
constexpr int64_t MaxEndOfFile = 0LL;
constexpr uint32_t BlockSize = 16 * 1024 * 1024;
}

WindowsFile::WindowsFile()
    : WindowsFile(INVALID_HANDLE_VALUE, false)
{
}

WindowsFile::WindowsFile(WindowsFile&& other) noexcept
    : WindowsFile(other.m_handle, other.m_readOnly)
{
    other.m_handle = INVALID_HANDLE_VALUE;
}

WindowsFile::WindowsFile(void* handle, bool readOnly)
    : m_handle(handle)
    , m_readOnly(readOnly)
    , m_lockProtocol {
        FileLockWindows { handle, 0, MaxEndOfFile - 3 },
        FileLockWindows { handle, MaxEndOfFile - 2, MaxEndOfFile - 1 },
        FileLockWindows { handle, MaxEndOfFile - 3, MaxEndOfFile - 2 },
    }
{
}

WindowsFile::WindowsFile(std::filesystem::path path, OpenMode mode)
    : WindowsFile(open(path, mode), mode == OpenMode::ReadOnly)
{
}

WindowsFile& WindowsFile::operator=(WindowsFile&& other) noexcept
{
    close();
    m_handle = other.m_handle;
    m_readOnly = other.m_readOnly;
    m_lockProtocol = std::move(other.m_lockProtocol);
    other.m_handle = INVALID_HANDLE_VALUE;
    return *this;
}

WindowsFile::~WindowsFile()
{
    close();
}

void WindowsFile::close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        [[maybe_unused]] auto succ = ::CloseHandle(m_handle);
        assert(succ);
    }
    m_handle = INVALID_HANDLE_VALUE;
}

void* TxFs::WindowsFile::open(std::filesystem::path path, OpenMode mode)
{
    void* handle = nullptr;
    switch (mode)
    {
    case OpenMode::Create:
        handle = Win32::CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

    case OpenMode::Open:
        handle = Win32::CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

         case OpenMode::ReadOnly:
             handle = Win32::CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
             break;

    default:
        throw std::runtime_error("WindowsFile::open(): unknown OpenMode");
    }

    return handle;
}

Interval WindowsFile::newInterval(size_t maxPages)
{
    LARGE_INTEGER size;
    Win32::GetFileSizeEx(m_handle, &size);
    Win32::Seek(m_handle, PageSize * maxPages + size.QuadPart);
    Win32::SetEndOfFile(m_handle);

    return Interval(static_cast<PageIndex>(size.QuadPart / PageSize),
                    static_cast<PageIndex>(size.QuadPart / PageSize + maxPages));
}

const uint8_t* WindowsFile::writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    if (pageOffset + (end - begin) > PageSize)
        throw std::runtime_error("WindowsFile::writePage over page boundary");
    if (fileSizeInPages() <= id)
        throw std::runtime_error("WindowsFile::writePage outside file");

    Win32::Seek(m_handle, PageSize * id + pageOffset);
    DWORD bytesWritten;
    Win32::WriteFile(m_handle, begin, static_cast<DWORD>(end - begin), &bytesWritten, nullptr);

    return end;
}

const uint8_t* WindowsFile::writePages(Interval iv, const uint8_t* page)
{
    if (fileSizeInPages() < iv.end())
        throw std::runtime_error("WindowsFile::writePages outside file");

    Win32::Seek(m_handle, PageSize * iv.begin());
    auto end = page + (iv.length() * PageSize);
    writePagesInBlocks(page, end);

    return end;
}

void TxFs::WindowsFile::writePagesInBlocks(const uint8_t* begin, const uint8_t* end)
{
    DWORD bytesWritten;
    for (; (begin + BlockSize) < end; begin += BlockSize)
        Win32::WriteFile(m_handle, begin, BlockSize, &bytesWritten, nullptr);

    Win32::WriteFile(m_handle, begin, static_cast<DWORD>(end - begin), &bytesWritten, nullptr);
}

uint8_t* WindowsFile::readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    if (pageOffset + (end - begin) > PageSize)
        throw std::runtime_error("WindowsFile::readPage over page boundary");
    if (fileSizeInPages() <= id)
        throw std::runtime_error("WindowsFile::readPage outside file");

    Win32::Seek(m_handle, PageSize * id + pageOffset);
    DWORD bytesRead;
    Win32::ReadFile(m_handle, begin, static_cast<DWORD>(end - begin), &bytesRead, nullptr);

    return begin + bytesRead;
}

uint8_t* WindowsFile::readPages(Interval iv, uint8_t* page) const
{
    if (fileSizeInPages() < iv.end())
        throw std::runtime_error("WindowsFile::readPages outside file");

    Win32::Seek(m_handle, PageSize * iv.begin());
    auto end = page + (iv.length() * PageSize);
    size_t bytesRead = readPagesInBlocks(page, end);

    return page + bytesRead;
}

size_t WindowsFile::readPagesInBlocks(uint8_t* begin, uint8_t* end) const
{
    DWORD bytesRead;
    size_t totalBytesRead = 0;
    for (; (begin + BlockSize) < end; begin += BlockSize)
    {
        Win32::ReadFile(m_handle, begin, BlockSize, &bytesRead, nullptr);
        totalBytesRead += bytesRead;
    }

    Win32::ReadFile(m_handle, begin, static_cast<DWORD>(end - begin), &bytesRead, nullptr);
    return totalBytesRead + bytesRead;
}

size_t WindowsFile::fileSizeInPages() const
{
    LARGE_INTEGER size;
    Win32::GetFileSizeEx(m_handle, &size);
    size.QuadPart += PageSize - 1;
    return static_cast<size_t>(size.QuadPart / PageSize); // pages rounded up
}

void WindowsFile::flushFile()
{
    Win32::FlushFileBuffers(m_handle);
}

void WindowsFile::truncate(size_t numberOfPages)
{
    Win32::Seek(m_handle, PageSize * numberOfPages);
    Win32::SetEndOfFile(m_handle);
}

Lock WindowsFile::defaultAccess()
{
    return m_readOnly ? readAccess() : writeAccess();
}

Lock WindowsFile::readAccess()
{
    return m_lockProtocol.readAccess();
}

Lock WindowsFile::writeAccess()
{
    return m_lockProtocol.writeAccess();
}

CommitLock WindowsFile::commitAccess(Lock&& writeLock)
{
    return m_lockProtocol.commitAccess(std::move(writeLock));
}

std::filesystem::path WindowsFile::getFileName() const
{
    std::wstring buffer(1028, 0);
    Win32::GetFinalPathNameByHandle(m_handle, buffer.data(), static_cast<DWORD>(buffer.size()), VOLUME_NAME_DOS);

    return buffer;
}
