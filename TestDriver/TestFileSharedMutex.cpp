
#include <gtest/gtest.h>
#include "CompoundFs/FileSharedMutex.h"
#include "FileLockingTester.h"

#include <filesystem>
#include <cstdio>
#include <string>
#include <mutex>
#include <shared_mutex>

#include <io.h>
#include <windows.h>

#pragma warning(disable : 4996) // disable "'tmpnam': This function or variable may be unsafe."

using namespace TxFs;

namespace
{
struct File
{
    File()
        : m_path(std::filesystem::temp_directory_path() / std::tmpnam(nullptr))
        , m_handle(::CreateFile(m_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr))
    {
    }

    File(const File& f)
        : m_path(f.m_path)
        , m_handle(::CreateFile(m_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr))
    {
    }

    File(nullptr_t)
        : m_handle(INVALID_HANDLE_VALUE)
    {
    }
    ~File() 
    { 
        ::CloseHandle(m_handle);
        if (!m_path.empty())
        {
            std::error_code ec;
            std::filesystem::remove(m_path, ec);
        }
    }

    std::filesystem::path m_path;
    HANDLE m_handle;
};
}

TEST(FileLockWindows, canWriteOnXLockedRange)
{
    File f;
    FileLockWindows fsm { f.m_handle, 0, -1 };
    fsm.lock();

    int handle = _open_osfhandle((intptr_t) f.m_handle, 0);
    auto res = _write(handle, "test", 5);
    ASSERT_EQ(res , 5);
}

TEST(FileLockWindows, cannotWriteOnSharedLockedRange)
{
    File f;
    FileLockWindows fsm { f.m_handle, 0, -1 };
    fsm.lock_shared();

    int handle = _open_osfhandle((intptr_t) f.m_handle, 0);
    auto res = _write(handle, "test", 5);
    ASSERT_EQ(res , -1);
}

TEST(FileLockWindows, othersCannotWriteOnLockedRange)
{
    bool exclusive = true;
    do
    {
        File f;
        FileLockWindows fsm { f.m_handle, 0, -1 };
        if (exclusive)
            fsm.lock();
        else
            fsm.lock_shared();

        // open file again
        File f2 = f;
        int handle = _open_osfhandle((intptr_t) f2.m_handle, 0);
        auto res = _write(handle, "test", 5);
        ASSERT_EQ(res , -1);
        exclusive = !exclusive;
    } while (exclusive == false);
}

namespace
{
    struct Helper
    {
        using File = File;
        using FileLock = FileLockWindows;
    };
}

INSTANTIATE_TYPED_TEST_SUITE_P(WindowsFileLocking, FileLockingTester, Helper);