
#include <gtest/gtest.h>
#include "CompoundFs/FileLockLinux.h"
#include "FileLockingTester.h"

#include <filesystem>
#include <cstdio>
#include <string>
#include <mutex>
#include <shared_mutex>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#pragma warning(disable : 4996) // disable "'tmpnam': This function or variable may be unsafe."

using namespace TxFs;

namespace
{
struct PhysicalFile
{
    PhysicalFile()
        : m_path(std::filesystem::temp_directory_path() / std::tmpnam(nullptr))
        , m_handle(::open(m_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666))
    {
    }

    PhysicalFile(const PhysicalFile& f)
        : m_path(f.m_path)
        , m_handle(::open(m_path.c_str(), O_CREAT | O_RDWR, 0666))
    {
    }

    PhysicalFile(std::nullptr_t)
        : m_handle(-1)
    {
    }
    ~PhysicalFile() 
    { 
        ::close(m_handle);
        if (!m_path.empty())
        {
            std::error_code ec;
            std::filesystem::remove(m_path, ec);
        }
    }

    std::filesystem::path m_path;
    int m_handle;
};
}

TEST(OpenFileDescriptorLock, throwOnInvalidHandle)
{
    //PhysicalFile f = nullptr;
    //FileSharedMutex fsm { f.m_handle, -2, -1 };
    //ASSERT_THROW(fsm.try_lock(), std::exception);
}


namespace
{
    struct Helper
    {
        using File = PhysicalFile;
        using FileLock = OpenFileDescriptorLock;
    };
}

INSTANTIATE_TYPED_TEST_SUITE_P(LinuxFileLocking, FileLockingTester, Helper);