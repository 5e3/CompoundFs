

#include <gtest/gtest.h>
#include <filesystem>
#include <cstdio>

#include "../CompoundFs/File.h"

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
}



