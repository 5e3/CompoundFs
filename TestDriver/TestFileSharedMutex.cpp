
#include <gtest/gtest.h>
#include "../CompoundFs/FileSharedMutex.h"

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
        ::DeleteFile(m_path.c_str());
    }

    std::filesystem::path m_path;
    HANDLE m_handle;
};
}

TEST(FileSharedMutex, throwOnInvalidHandle)
{
    File f = nullptr;
    FileSharedMutex fsm { f.m_handle, (uint64_t) -2, (uint64_t) -1 };

    try
    {
        bool succ = fsm.try_lock();
        ASSERT_TRUE(false);
    }
    catch (const std::exception& e)
    {
        std::string msg = e.what();
    }
}

TEST(FileSharedMutex, throwOnEmptyRange)
{
    File f = nullptr;
    FileSharedMutex fsm { f.m_handle, (uint64_t) -2, (uint64_t) -2 };

    try
    {
        bool succ = fsm.try_lock();
        ASSERT_TRUE(false);
    }
    catch (const std::exception&)
    {}
}

TEST(FileSharedMutex, canWriteOnXLockedRange)
{
    File f;
    FileSharedMutex fsm { f.m_handle, (uint64_t) 0, (uint64_t) -1 };
    fsm.lock();

    int handle = _open_osfhandle((intptr_t) f.m_handle, 0);
    auto res = _write(handle, "test", 5);
    ASSERT_TRUE(res == 5);
}

TEST(FileSharedMutex, cannotWriteOnSharedLockedRange)
{
    File f;
    FileSharedMutex fsm { f.m_handle, (uint64_t) 0, (uint64_t) -1 };
    fsm.lock_shared();

    int handle = _open_osfhandle((intptr_t) f.m_handle, 0);
    auto res = _write(handle, "test", 5);
    ASSERT_TRUE(res == -1);
}

TEST(FileSharedMutex, othersCannotWriteOnLockedRange)
{
    bool exclusive = true;
    do
    {
        File f;
        FileSharedMutex fsm { f.m_handle, (uint64_t) 0, (uint64_t) -1 };
        if (exclusive)
            fsm.lock();
        else
            fsm.lock_shared();

        // open file again
        File f2 = f;
        int handle = _open_osfhandle((intptr_t) f2.m_handle, 0);
        auto res = _write(handle, "test", 5);
        ASSERT_TRUE(res == -1);
        exclusive = !exclusive;
    } while (exclusive == false);
}

TEST(FileSharedMutex, XLockPreventsOtherLocks)
{
    File f;
    FileSharedMutex fsm { f.m_handle, (uint64_t) -2, (uint64_t) -1 };
    std::unique_lock lock(fsm);

    File f2 = f;
    FileSharedMutex fsm2 { f2.m_handle, (uint64_t) -2, (uint64_t) -1 };
    ASSERT_TRUE(!fsm2.try_lock_shared());
    ASSERT_TRUE(!fsm2.try_lock());
}

TEST(FileSharedMutex, SLockPreventsXLock)
{
    File f;
    FileSharedMutex fsm { f.m_handle, (uint64_t) -2, (uint64_t) -1 };
    std::shared_lock lock(fsm);

    File f2 = f;
    FileSharedMutex fsm2 { f2.m_handle, (uint64_t) -2, (uint64_t) -1 };
    ASSERT_TRUE(!fsm2.try_lock());
    ASSERT_TRUE(fsm2.try_lock_shared());
}
