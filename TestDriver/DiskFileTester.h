
#pragma once

#include <gtest/gtest.h>
#include <filesystem>
#include <cstdio>
#include <system_error>
#include <numeric>
#include <fstream>
#include <thread>

#include "CompoundFs/FileInterface.h"
#include "CompoundFs/ByteString.h"
#include "CompoundFs/Lock.h"

namespace TxFs
{

template <typename TDiskFile>
struct DiskFileTester : ::testing::Test
{
    std::filesystem::path m_tempFileName;

    DiskFileTester()
        : m_tempFileName(std::filesystem::temp_directory_path() / std::tmpnam(nullptr))
    {
    }

    void prepareFileWithContents(std::string_view sv)
    {
        std::ofstream out(m_tempFileName);
        out << sv;
    }

    ~DiskFileTester()
    {
        std::error_code errorCode;
        std::filesystem::remove(m_tempFileName, errorCode);
    }

    class File : public TDiskFile
    {
    public:
        File(std::filesystem::path path, OpenMode mode)
            : TDiskFile(path, mode)
        {
        }
        
        bool tryGetReadAccess()
        {
            return m_lockProtocol.tryReadAccess().has_value();
        }
    };
};

TYPED_TEST_SUITE_P(DiskFileTester);

TYPED_TEST_P(DiskFileTester, illegalFileNamesThrow)
{
    ASSERT_THROW(TypeParam f("<?* />.txt", OpenMode::CreateAlways), std::system_error);
    ASSERT_THROW(TypeParam f("<?* />.txt", OpenMode::Open), std::system_error);
    ASSERT_THROW(TypeParam f("<?* />.txt", OpenMode::ReadOnly), std::system_error);
}

TYPED_TEST_P(DiskFileTester, openNonExistantFilesThrows)
{
    ASSERT_THROW(TypeParam f(this->m_tempFileName, OpenMode::ReadOnly), std::system_error);
}

TYPED_TEST_P(DiskFileTester, createTruncatesFile)
{
    TypeParam file(this->m_tempFileName, OpenMode::CreateAlways);
    file.newInterval(5);
    TypeParam file2 = TypeParam(this->m_tempFileName, OpenMode::CreateAlways);
    ASSERT_EQ(file.fileSizeInPages(), 0);
    ASSERT_EQ(file2.fileSizeInPages(), 0);
}

TYPED_TEST_P(DiskFileTester, truncateHoldsCommitLock)
{
    File file(this->m_tempFileName, OpenMode::CreateAlways);
    file.newInterval(5);
    auto lock = file.readAccess();

    std::thread t ([this]() { TypeParam file2 = TypeParam(this->m_tempFileName, OpenMode::CreateAlways); });
    File file3(this->m_tempFileName, OpenMode::OpenExisting);
    while (file3.fileSizeInPages() > 0 && file3.tryGetReadAccess())
    {
        std::this_thread::yield();
    }

    EXPECT_NE(file.fileSizeInPages(), 0);
    lock.release();
    t.join();
    EXPECT_EQ(file.fileSizeInPages(), 0);
}

TYPED_TEST_P(DiskFileTester, createNewThrowsIfFileExists)
{
    {
        TypeParam file(this->m_tempFileName, OpenMode::CreateNew);
    }

    ASSERT_THROW(TypeParam file(this->m_tempFileName, OpenMode::CreateNew), std::exception);
}

TYPED_TEST_P(DiskFileTester, openExistingThrowsIfFileDoesNotExists)
{
    ASSERT_THROW(TypeParam file(this->m_tempFileName, OpenMode::OpenExisting), std::exception);
    {
        TypeParam file(this->m_tempFileName, OpenMode::CreateAlways);
    }

    TypeParam file(this->m_tempFileName, OpenMode::OpenExisting);
}

TYPED_TEST_P(DiskFileTester, overflowingOpenModeThrows)
{
    ASSERT_THROW(TypeParam file(this->m_tempFileName, OpenMode(5)), std::exception);
}

TYPED_TEST_P(DiskFileTester, canOpenSameFileMoreThanOnce)
{
    TypeParam file(this->m_tempFileName, OpenMode::CreateAlways);
    {
        TypeParam file1(this->m_tempFileName, OpenMode::Open);
        TypeParam file2(this->m_tempFileName, OpenMode::ReadOnly);
    }
    file = TypeParam();
}

TYPED_TEST_P(DiskFileTester, uninitializedFileThrows)
{
    TypeParam file;
    ASSERT_THROW(file.newInterval(2), std::exception);
    ASSERT_THROW(file.fileSizeInPages(), std::exception);
    ASSERT_THROW(file.flushFile(), std::exception);
    ASSERT_THROW(file.truncate(0), std::exception);
    ASSERT_THROW(file.readAccess(), std::exception);
    ASSERT_THROW(file.writeAccess(), std::exception);
}

TYPED_TEST_P(DiskFileTester, readOnlyFileThrowsOnWriteOps)
{
    {
        TypeParam wfile(this->m_tempFileName, OpenMode::CreateAlways);
        wfile.newInterval(5);
    }

    auto file = TypeParam(this->m_tempFileName, OpenMode::ReadOnly);
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

    TypeParam file(this->m_tempFileName, OpenMode::CreateAlways);
    auto iv = file.newInterval(out.size() * sizeof(uint64_t) / 4096);
    file.writePages(iv, (const uint8_t*) out.data());

    std::vector<uint64_t> in(out.size());
    file.readPages(iv, (uint8_t*) in.data());
    ASSERT_EQ(out, in);
}

TYPED_TEST_P(DiskFileTester, nonEmptyFileReportsRoundedupSize)
{
    this->prepareFileWithContents("1234");
    TypeParam file(this->m_tempFileName, OpenMode::Open);
    ASSERT_EQ(file.fileSizeInPages(), 1); // char[4] rounded up => 1 page
}

TYPED_TEST_P(DiskFileTester, readPageOverEndOfFileReturnsBufferPosition)
{
    std::string_view data("1234");
    this->prepareFileWithContents(data);
    TypeParam file(this->m_tempFileName, OpenMode::ReadOnly);

    uint8_t buf[4096];
    ASSERT_EQ(file.readPage(0, 0, buf, buf + sizeof(buf)), buf + data.size());
}

TYPED_TEST_P(DiskFileTester, readPagesOverEndOfFileReturnsBufferPosition)
{
    std::string_view data("1234");
    this->prepareFileWithContents(data);
    TypeParam file(this->m_tempFileName, OpenMode::ReadOnly);

    uint8_t buf[4096];
    ASSERT_EQ(file.readPages(Interval(0, 1), buf), buf + data.size()); // different api than previous test
}

REGISTER_TYPED_TEST_SUITE_P(DiskFileTester, illegalFileNamesThrow, openNonExistantFilesThrows, createTruncatesFile,
                            truncateHoldsCommitLock, createNewThrowsIfFileExists, openExistingThrowsIfFileDoesNotExists,
                            overflowingOpenModeThrows, canOpenSameFileMoreThanOnce, uninitializedFileThrows,
                            readOnlyFileThrowsOnWriteOps, canReadWriteBigPages, nonEmptyFileReportsRoundedupSize,
                            readPageOverEndOfFileReturnsBufferPosition, readPagesOverEndOfFileReturnsBufferPosition);

}