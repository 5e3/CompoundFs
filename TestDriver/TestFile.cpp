

#include <gtest/gtest.h>
#include <filesystem>
#include <cstdio>
#include <numeric>

#include "CompoundFs/File.h"
#include "CompoundFs/ByteString.h"

#pragma warning(disable : 4996) // disable "'tmpnam': This function or variable may be unsafe."

using namespace TxFs;

namespace
{
std::filesystem::path createFileName()
{
    return std::filesystem::temp_directory_path() / std::tmpnam(nullptr);
}
}


TEST(File, illegalFileNamesThrow)
{
    ASSERT_THROW(File f("*::", OpenMode::Create), std::system_error);
    ASSERT_THROW(File f("*::", OpenMode::Open), std::system_error);
    ASSERT_THROW(File f("*::", OpenMode::ReadOnly), std::system_error);
}

TEST(File, openNonExistantFilesThrows)
{
    auto path = createFileName();
    ASSERT_THROW(File f(path, OpenMode::ReadOnly), std::system_error);
}

TEST(File, createTruncatesFile)
{
    auto path = createFileName();
    File file(path, OpenMode::Create);
    file.newInterval(5);
    file = File(path, OpenMode::Create);
    ASSERT_EQ(file.currentSize(), 0);
    file = File();
    std::filesystem::remove(path);
}

TEST(File, canOpenSameFileMoreThanOnce)
{
    auto path = createFileName();
    File file(path, OpenMode::Create);
    {
        File file1(path, OpenMode::Open);
        File file2(path, OpenMode::ReadOnly);
    }
    file = File();
    std::filesystem::remove(path);
}

TEST(File, uninitializedFileThrows)
{
    File file;
    ASSERT_THROW(file.newInterval(2), std::exception);
    ASSERT_THROW(file.currentSize(), std::exception);
    ASSERT_THROW(file.flushFile(), std::exception);
    ASSERT_THROW(file.truncate(0), std::exception);
    ASSERT_THROW(file.readAccess(), std::exception);
    ASSERT_THROW(file.writeAccess(), std::exception);
    ASSERT_THROW(file.getFileName(), std::exception);
}

TEST(File, readOnlyFileThrowsOnWriteOps)
{
    TempFile wfile;
    wfile.newInterval(5);

    auto file = File(wfile.getFileName(), OpenMode::ReadOnly);
    ASSERT_THROW(file.newInterval(2), std::exception);
    ASSERT_THROW(file.truncate(0), std::exception);
    ByteStringView out("0123456789");
    ASSERT_THROW(file.writePage(1, 0, out.data(), out.end()), std::exception);
}

TEST(File, canWriteOnWriteLockedFile)
{
    auto wfile = TempFile();
    auto rfile = File(wfile.getFileName(), OpenMode::ReadOnly);
    auto rlock = rfile.defaultAccess();
    auto wlock = wfile.defaultAccess();
    wfile.newInterval(5);
    ByteStringView out("0123456789");
    wfile.writePage(1, 0, out.data(), out.end());
    ASSERT_NO_THROW(wfile.writePage(1, 0, out.data(), out.end()));
}

TEST(File, canReadWriteBigPages)
{
    // File.cpp BlockSize=16MegaByte
    std::vector<uint64_t> out((16 * 3 * 1024 * 1024 - 4096) / sizeof(uint64_t));
    std::iota(out.begin(), out.end(), 0);

    TempFile file;
    auto iv = file.newInterval(out.size() * sizeof(uint64_t) / 4096);
    file.writePages(iv, (const uint8_t*) out.data());

    std::vector<uint64_t> in(out.size());
    file.readPages(iv, (uint8_t*) in.data());
    ASSERT_EQ(out, in);
}



