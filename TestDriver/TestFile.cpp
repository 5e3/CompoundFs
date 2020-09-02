

#include <gtest/gtest.h>
#include <filesystem>
#include <cstdio>

#include "../CompoundFs/File.h"
#include "../CompoundFs/ByteString.h"

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
    ASSERT_THROW(File::create("*::"), std::system_error);
    ASSERT_THROW(File::open("*::"), std::system_error);
    ASSERT_THROW(File::open("*::", true), std::system_error);
}

TEST(File, openNonExistantFilesThrows)
{
    auto path = createFileName();
    ASSERT_THROW(File::open(path, true), std::system_error);
}

TEST(File, createTruncatesFile)
{
    auto path = createFileName();
    auto file = File::create(path);
    file.newInterval(5);
    file = File::create(path);
    ASSERT_EQ(file.currentSize(), 0);
    file = File();
    std::filesystem::remove(path);
}

TEST(File, canOpenSameFileMoreThanOnce)
{
    auto path = createFileName();
    auto file = File::create(path);
    File::open(path);
    File::open(path, true);
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
    auto wfile = File::createTemp();
    wfile.newInterval(5);

    auto file = File::open(wfile.getFileName(), true);
    ASSERT_THROW(file.newInterval(2), std::exception);
    ASSERT_THROW(file.truncate(0), std::exception);
    ByteStringView out("0123456789");
    ASSERT_THROW(file.writePage(1, 0, out.data(), out.end()), std::exception);
    auto path = wfile.getFileName();
    wfile = File();
    file = File();
    std::filesystem::remove(path);
}

TEST(File, canWriteOnWriteLockedFile)
{
    auto wfile = File::createTemp();
    auto rfile = File::open(wfile.getFileName(), true);
    {
        auto rlock = rfile.defaultAccess();
        auto wlock = wfile.defaultAccess();
        wfile.newInterval(5);
        ByteStringView out("0123456789");
        wfile.writePage(1, 0, out.data(), out.end());
        ASSERT_NO_THROW(wfile.writePage(1, 0, out.data(), out.end()));
    }
    auto path = wfile.getFileName();
    wfile = File();
    rfile = File();
    std::filesystem::remove(path);
}



