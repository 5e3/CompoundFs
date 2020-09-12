

#include <gtest/gtest.h>

#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/File.h"
#include "CompoundFs/ByteString.h"

using namespace TxFs;

template<typename T>
struct FileInterfaceTester : ::testing::Test
{
    FileInterfaceTester()
        : m_fileInterface(new T)
    {

    }

    ~FileInterfaceTester() { delete m_fileInterface; }

    FileInterface* m_fileInterface = nullptr;

};

using FileIntefaceTypes = ::testing::Types<MemoryFile, TempFile>;
TYPED_TEST_SUITE(FileInterfaceTester, FileIntefaceTypes);

TYPED_TEST(FileInterfaceTester, newlyCreatedFileIsEmpty)
{
    ASSERT_EQ(this->m_fileInterface->currentSize(), 0);
}

TYPED_TEST(FileInterfaceTester, newIntervalReturnsCorrespondingInterval)
{
    auto interval = this->m_fileInterface->newInterval(5);
    ASSERT_EQ(interval, Interval(0, 5));
    interval = this->m_fileInterface->newInterval(5);
    ASSERT_EQ(interval, Interval(5, 10));
}

TYPED_TEST(FileInterfaceTester, truncateReducesFileSize)
{
    this->m_fileInterface->newInterval(5);
    this->m_fileInterface->truncate(5);
    ASSERT_EQ(this->m_fileInterface->currentSize(), 5);

    this->m_fileInterface->truncate(2);
    ASSERT_EQ(this->m_fileInterface->currentSize(), 2);
}

TYPED_TEST(FileInterfaceTester, readWriteOutsideCurrentFileSizeThrows)
{
    uint8_t buf[4096];
    this->m_fileInterface->newInterval(5);

    ASSERT_THROW(this->m_fileInterface->readPage(5, 0, buf, buf + 1), std::exception);
    ASSERT_THROW(this->m_fileInterface->writePage(5, 0, buf, buf + 1), std::exception);
    ASSERT_THROW(this->m_fileInterface->readPages(Interval(5), buf), std::exception);
    ASSERT_THROW(this->m_fileInterface->writePages(Interval(5), buf), std::exception);
}

TYPED_TEST(FileInterfaceTester, readWritePageOverPageBounderiesThrows)
{
    uint8_t buf[4096];
    this->m_fileInterface->newInterval(5);

    ASSERT_THROW(this->m_fileInterface->readPage(1, 4095, buf, buf + 2), std::exception);
    ASSERT_THROW(this->m_fileInterface->writePage(1, 4095, buf, buf + 2), std::exception);
}

TYPED_TEST(FileInterfaceTester, readPagesReturnsDataOfWritePages)
{
    std::string outString(3 * 4096, 'X');
    auto begin = (uint8_t*) outString.data();
    this->m_fileInterface->newInterval(5);

    ASSERT_EQ(this->m_fileInterface->writePages(Interval(1, 4), begin), begin+outString.size());

    std::string inString(3 * 4096, 'Y');
    begin = (uint8_t*) inString.data();
    ASSERT_EQ(this->m_fileInterface->readPages(Interval(1, 4), begin), begin + inString.size());
    ASSERT_EQ(inString, outString);
}

TYPED_TEST(FileInterfaceTester, readPageReturnsDataOfWritePage)
{
    std::string outString(3 * 4096, 'X');
    auto begin = (uint8_t*) outString.data();
    this->m_fileInterface->newInterval(5);
    this->m_fileInterface->writePages(Interval(1, 4), begin);

    ByteStringView out("0123456789");
    ASSERT_EQ(this->m_fileInterface->writePage(1, 0, out.data(), out.end()), out.end());
    uint8_t in[11];
    ASSERT_EQ(this->m_fileInterface->readPage(1, 0, in, in + sizeof(in)), in+sizeof(in));
    ASSERT_EQ("0123456789X", ByteStringView(in, sizeof(in)));

    ASSERT_EQ(this->m_fileInterface->readPage(2, 100, in, in + sizeof(in)), in + sizeof(in));
    ASSERT_EQ("XXXXXXXXXXX", ByteStringView(in, sizeof(in)));
    ASSERT_EQ(this->m_fileInterface->writePage(2, 100, out.data(), out.end()), out.end());
    ASSERT_EQ(this->m_fileInterface->readPage(2, 100, in, in + sizeof(in)), in + sizeof(in));
    ASSERT_EQ("0123456789X", ByteStringView(in, sizeof(in)));

    ASSERT_EQ(this->m_fileInterface->readPage(3, 4096-11, in, in + sizeof(in)), in + sizeof(in));
    ASSERT_EQ("XXXXXXXXXXX", ByteStringView(in, sizeof(in)));
    ASSERT_EQ(this->m_fileInterface->writePage(3, 4096-10, out.data(), out.end()), out.end());
    ASSERT_EQ(this->m_fileInterface->readPage(3, 4096 - 11, in, in + sizeof(in)), in + sizeof(in));
    ASSERT_EQ("X0123456789", ByteStringView(in, sizeof(in)));
}














