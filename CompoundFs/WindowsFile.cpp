

#include "WindowsFile.h"
#include "FileLockPosition.h"
#define NOMINMAX
#include "windows.h"

#include <utility>
#include <filesystem>
#include <cstdio>
#include <array>
#include <limits>

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

template<auto TFunc>
struct WrapOsCall
{
    template <typename... TArgs>
    auto operator()(TArgs&&... args) const
    {
        auto ret = TFunc(std::forward<TArgs>(args)...);
        throwOnError(ret);
        return ret;
    }
};


constexpr auto GetFileSizeEx = WrapOsCall<::GetFileSizeEx>();
constexpr auto CreateFile = WrapOsCall<::CreateFile>();
constexpr auto SetFilePointerEx = WrapOsCall <::SetFilePointerEx>();
constexpr auto SetEndOfFile = WrapOsCall<::SetEndOfFile>();
constexpr auto WriteFile = WrapOsCall<::WriteFile>();
constexpr auto ReadFile = WrapOsCall<::ReadFile>();
constexpr auto FlushFileBuffers = WrapOsCall<::FlushFileBuffers>();
constexpr auto GetFinalPathNameByHandle = WrapOsCall<::GetFinalPathNameByHandle>();

void Seek(HANDLE handle, uint64_t position)
{
    LARGE_INTEGER pos;
    pos.QuadPart = position;
    Win32::SetFilePointerEx(handle, pos, nullptr, FILE_BEGIN);
}
}

namespace
{
constexpr uint64_t PageSize = 4096ULL;
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
        FileLockWindows { handle, FileLockPosition::GateBegin, FileLockPosition::GateEnd},
        FileLockWindows { handle, FileLockPosition::SharedBegin, FileLockPosition::SharedEnd},
        FileLockWindows { handle, FileLockPosition::WriteBegin, FileLockPosition::WriteEnd},
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

WindowsFile::WindowsFile(std::filesystem::path path, OpenMode mode)
    : WindowsFile(open(path, mode), mode == OpenMode::ReadOnly)
{
    if (mode == OpenMode::CreateAlways)
    {   // truncate with commit lock
        auto lock = commitAccess(writeAccess());
        truncate(0);
        lock.release();
    }
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

namespace
{

DWORD mapToWindowsFileModes(OpenMode mode)
{
    switch (mode)
    {
    case OpenMode::CreateNew:
        return CREATE_NEW;
    case OpenMode::CreateAlways:
        return OPEN_ALWAYS; // open without truncate
    case OpenMode::Open:
        return OPEN_ALWAYS;
    case OpenMode::OpenExisting:
        return OPEN_EXISTING;
    case OpenMode::ReadOnly:
        return OPEN_EXISTING;
    default:
        throw std::runtime_error("Unknown OpenMode");
    }
}

DWORD mapToWindowsAccessFlags(OpenMode mode)
{
    if (mode == OpenMode::ReadOnly)
        return GENERIC_READ;
    return GENERIC_READ | GENERIC_WRITE;
}

}

void* TxFs::WindowsFile::open(std::filesystem::path path, OpenMode mode)
{
    return Win32::CreateFile(path.c_str(), mapToWindowsAccessFlags(mode), FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                             mapToWindowsFileModes(mode), FILE_ATTRIBUTE_NORMAL, nullptr);
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
