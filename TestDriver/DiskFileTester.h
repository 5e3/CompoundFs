
#include <gtest/gtest.h>
#include <filesystem>
#include <cstdio>
#include <system_error>
#include <numeric>

#include "CompoundFs/ByteString.h"
#include "CompoundFs/Lock.h"

namespace TxFs
{

template <typename TDiskFile>
struct DiskFileTester : ::testing::Test
{
    std::filesystem::path m_tempFile;

    DiskFileTester()
        : m_tempFile(std::filesystem::temp_directory_path() / std::tmpnam(nullptr))
    {

    }

    ~DiskFileTester()  
    {
        std::error_code errorCode;
        std::filesystem::remove(m_tempFile, errorCode);
    }
};

TYPED_TEST_SUITE_P(DiskFileTester);

TYPED_TEST_P(DiskFileTester, illegalFileNamesThrow)
{
    ASSERT_THROW(TypeParam f("*::", OpenMode::Create), std::system_error);
    ASSERT_THROW(TypeParam f("*::", OpenMode::Open), std::system_error);
    ASSERT_THROW(TypeParam f("*::", OpenMode::ReadOnly), std::system_error);
}

TYPED_TEST_P(DiskFileTester, openNonExistantFilesThrows)
{
    ASSERT_THROW(TypeParam f(m_tempFile, OpenMode::ReadOnly), std::system_error);
}



TYPED_TEST_P(DiskFileTester, createTruncatesFile)
{
    TypeParam file(m_tempFile, OpenMode::Create);
    file.newInterval(5);
    file = TypeParam(m_tempFile, OpenMode::Create);
    ASSERT_EQ(file.currentSize(), 0);
    file = TypeParam();
}

TYPED_TEST_P(DiskFileTester, canOpenSameFileMoreThanOnce)
{
    TypeParam file(m_tempFile, OpenMode::Create);
    {
        TypeParam file1(m_tempFile, OpenMode::Open);
        TypeParam file2(m_tempFile, OpenMode::ReadOnly);
    }
    file = TypeParam();
}


TYPED_TEST_P(DiskFileTester, uninitializedFileThrows)
{
    TypeParam file;
    ASSERT_THROW(file.newInterval(2), std::exception);
    ASSERT_THROW(file.currentSize(), std::exception);
    ASSERT_THROW(file.flushFile(), std::exception);
    ASSERT_THROW(file.truncate(0), std::exception);
    ASSERT_THROW(file.readAccess(), std::exception);
    ASSERT_THROW(file.writeAccess(), std::exception);
}

TYPED_TEST_P(DiskFileTester, readOnlyFileThrowsOnWriteOps)
{
    {
        TypeParam wfile(m_tempFile, OpenMode::Create);
        wfile.newInterval(5);
    }

    auto file = TypeParam(m_tempFile, OpenMode::ReadOnly);
    ASSERT_THROW(file.newInterval(2), std::exception);
    ASSERT_THROW(file.truncate(0), std::exception);
    ByteStringView out("0123456789");
    ASSERT_THROW(file.writePage(1, 0, out.data(), out.end()), std::exception);
}

TYPED_TEST_P(DiskFileTester, canReadWriteBigPages)
{
    // File.cpp BlockSize=16MegaByte
    std::vector<uint64_t> out((16 * 3 * 1024 * 1024 - 4096) / sizeof(uint64_t));
    std::iota(out.begin(), out.end(), 0);

    TypeParam file(m_tempFile, OpenMode::Create);
    auto iv = file.newInterval(out.size() * sizeof(uint64_t) / 4096);
    file.writePages(iv, (const uint8_t*) out.data());

    std::vector<uint64_t> in(out.size());
    file.readPages(iv, (uint8_t*) in.data());
    ASSERT_EQ(out, in);
}

REGISTER_TYPED_TEST_SUITE_P(DiskFileTester, illegalFileNamesThrow, openNonExistantFilesThrows, createTruncatesFile,
                            canOpenSameFileMoreThanOnce, uninitializedFileThrows, readOnlyFileThrowsOnWriteOps,
                            canReadWriteBigPages);

}