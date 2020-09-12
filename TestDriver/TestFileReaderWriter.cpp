
#include <gtest/gtest.h>

#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/FileReader.h"
#include "CompoundFs/FileWriter.h"
#include "CompoundFs/ByteString.h"
#include <string>
#include <algorithm>
#include <random>

using namespace TxFs;

TEST(FileWriter, CtorCreatesEmptyFileDesc)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());

    FileWriter f(cm);
    FileDescriptor fd = f.close();
    ASSERT_EQ(fd , FileDescriptor());

    f.write(0, 0);
    fd = f.close();
    ASSERT_EQ(fd , FileDescriptor());
}

TEST(FileWriter, WriteCloseCreatesFileDescriptor)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());

    FileWriter f(cm);
    ByteStringView data("Test");
    f.write(data.data(), data.end());
    FileDescriptor fd = f.close();

    ASSERT_NE(fd , FileDescriptor());
    ASSERT_EQ(fd.m_fileSize , data.size());
    ASSERT_EQ(fd.m_first , fd.m_last);
}

TEST(FileWriter, WriteCloseCreatesFileTablePage)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    ByteStringView data("Test");
    f.write(data.data(), data.end());
    FileDescriptor fd = f.close();
    ASSERT_EQ(fd.m_first , fd.m_last);

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    ASSERT_EQ(fileTablePage.m_index , fd.m_last);

    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.size() , 1);
    ASSERT_EQ(is.front() , Interval(0, 1));
}

TEST(FileWriter, MultipleWritesCreatesDescOfAppropriateSize)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());

    FileWriter f(cm);
    ByteStringView data("Test");

    for (int i = 0; i < 10; i++)
        f.write(data.data(), data.end());

    FileDescriptor fd = f.close();

    ASSERT_NE(fd , FileDescriptor());
    ASSERT_EQ(fd.m_fileSize , 10ULL * data.size());
    ASSERT_EQ(fd.m_first , fd.m_last);
}

TEST(FileWriter, MultipleOpenCreatesDescOfAppropriateSize)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());

    FileWriter f(cm);
    ByteStringView data("Test");

    for (int i = 0; i < 10; i++)
        f.write(data.data(), data.end());

    FileDescriptor fd = f.close();

    f.openAppend(fd);
    for (int i = 0; i < 10; i++)
        f.write(data.data(), data.end());

    fd = f.close();

    ASSERT_NE(fd , FileDescriptor());
    ASSERT_EQ(fd.m_fileSize , 20ULL * data.size());
    ASSERT_EQ(fd.m_first , fd.m_last);
}

TEST(FileWriter, SmallWritesOverPageBoundery)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    ByteStringView data("Test0");
    for (int i = 0; i < 1000; i++)
        f.write(data.data(), data.end());

    FileDescriptor fd = f.close();
    ASSERT_EQ(fd.m_first , fd.m_last);
    ASSERT_EQ(fd.m_fileSize , 1000ULL * data.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    ASSERT_EQ(fileTablePage.m_index , fd.m_last);

    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.size() , 1);
    ASSERT_EQ(is.front() , Interval(0, 2));
}

TEST(FileWriter, SmallWritesWithAppendOverPageBoundery)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    ByteStringView data("Test0");
    for (int i = 0; i < 1000; i++)
        f.write(data.data(), data.end());

    FileDescriptor fd = f.close();
    ASSERT_EQ(fd.m_first , fd.m_last);
    ASSERT_EQ(fd.m_fileSize , 1000ULL * data.size());

    f.openAppend(fd);
    for (int i = 0; i < 1000; i++)
        f.write(data.data(), data.end());

    fd = f.close();
    ASSERT_EQ(fd.m_first , fd.m_last);
    ASSERT_EQ(fd.m_fileSize , 2000ULL * data.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    ASSERT_EQ(fileTablePage.m_index , fd.m_last);

    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.size() , 2);
    ASSERT_EQ(is.front() , Interval(0, 2));
    ASSERT_EQ(is.back() , Interval(3, 4));
}

TEST(FileWriter, PageSizeWrites)
{
    std::string str(4096, 'X');
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    ASSERT_EQ(fd.m_first , fd.m_last);
    ASSERT_EQ(fd.m_fileSize , 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.front() , Interval(0, 10));
}

TEST(FileWriter, OverPageSizeWrites)
{
    std::string str(4097, 'X');
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    ASSERT_EQ(fd.m_first , fd.m_last);
    ASSERT_EQ(fd.m_fileSize , 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.front() , Interval(0, 11));
}

TEST(FileWriter, UnderPageSizeWrites)
{
    std::string str(4095, 'X');
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    ASSERT_EQ(fd.m_first , fd.m_last);
    ASSERT_EQ(fd.m_fileSize , 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.front() , Interval(0, 10));
}

TEST(FileWriter, LargePageSizeWrites)
{
    std::string str(8192, 'X');
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    ASSERT_EQ(fd.m_first , fd.m_last);
    ASSERT_EQ(fd.m_fileSize , 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.front() , Interval(0, 20));
}

TEST(FileWriter, LargeSizeWrites)
{
    std::string str(20000, 'X');
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    ASSERT_EQ(fd.m_first , fd.m_last);
    ASSERT_EQ(fd.m_fileSize , 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.front() , Interval(0, uint32_t(10 * str.size() / 4096 + 1)));
}

TEST(FileWriter, LargeSizeWritesMultiFiles)
{
    std::string str(20000, 'X');
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    FileWriter f2(cm);
    for (int i = 0; i < 10; i++)
    {
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());
        f2.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());
    }

    FileDescriptor fd = f.close();
    FileDescriptor fd2 = f2.close();
    ASSERT_EQ(fd.m_fileSize , 10ULL * str.size());
    ASSERT_EQ(fd2.m_fileSize , 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    auto fileTablePage2 = tcm.loadPage<FileTable>(fd2.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.front() , Interval(0, uint32_t(str.size() / 4096 + 1)));

    IntervalSequence is2;
    fileTablePage2.m_page->insertInto(is2);
    ASSERT_EQ(is2.totalLength() , 10 * str.size() / 4096 + 1);
}

TEST(FileWriter, FillPageTable)
{
    std::string str(4096, 'X');
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    FileWriter f2(cm);
    for (int i = 0; i < 2000; i++)
    {
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());
        f2.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());
    }

    FileDescriptor fd = f.close();
    FileDescriptor fd2 = f2.close();
    ASSERT_EQ(fd.m_fileSize , 2000ULL * str.size());
    ASSERT_NE(fd.m_first , fd.m_last);

    ASSERT_EQ(fd2.m_fileSize , 2000ULL * str.size());
    ASSERT_NE(fd2.m_first , fd2.m_last);

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_first);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.size() , 2000);

    fileTablePage = tcm.loadPage<FileTable>(fd2.m_first);
    fileTablePage.m_page->insertInto(is);
    fileTablePage = tcm.loadPage<FileTable>(fd2.m_last);
    fileTablePage.m_page->insertInto(is);
    ASSERT_EQ(is.size() , 4000);
}

TEST(FileReader, ReadNullFile)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    FileReader f(cm);

    ASSERT_EQ(f.read(0, 0), nullptr);

    uint8_t buf;
    ASSERT_EQ(f.read(&buf, &buf + 1) , &buf);
    ASSERT_EQ(f.bytesLeft() , 0);
}

std::vector<uint8_t> makeRandomVector(size_t size)
{
    std::vector<uint8_t> v(size);
    for (size_t i = 0; i < size; i++)
        v[i] = uint8_t(i);
    std::shuffle(v.begin(), v.end(), std::mt19937(std::random_device()()));
    return v;
}

FileDescriptor writeFragmentedFile(const std::vector<uint8_t>& v, std::shared_ptr<CacheManager> cacheManager)
{
    FileWriter fr(cacheManager);
    FileWriter fr2(cacheManager);
    size_t s = (v.size() / 4096) * 4096;
    auto it = v.begin();
    for (auto it = v.begin(); it < (v.begin() + s); it += 4096)
    {
        fr.writeIterator(it, it + 4096);
        fr2.writeIterator(it, it + 4096);
    }

    fr.writeIterator(v.begin() + s, v.end());
    fr2.writeIterator(v.begin() + s, v.end());
    fr2.close();
    return fr.close();
}

std::vector<uint8_t> makeVector(size_t size)
{
    std::vector<uint8_t> v(size);
    int i = 0;
    for (auto& elem: v)
    {
        elem = i++;
        if (i >= 251)
            i = 0;
    }
    return v;
}

FileDescriptor writeFile(const std::vector<uint8_t>& v, std::shared_ptr<CacheManager> cacheManager)
{
    FileWriter fr(cacheManager);
    fr.writeIterator(v.begin(), v.end());
    return fr.close();
}

uint8_t* readFile(FileDescriptor fd, std::vector<uint8_t>& v, std::shared_ptr<CacheManager> cacheManager, size_t sizes)
{
    FileReader fr(cacheManager);
    fr.open(fd);
    uint8_t* begin = &v[0];
    while (fr.bytesLeft())
    {
        begin = fr.read(begin, begin + sizes);
    }
    return begin;
}

TEST(FileReader, ReadSmallFile)
{
    std::vector<uint8_t> v = makeRandomVector(1000);
    auto cacheManager = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    FileDescriptor fd = writeFile(v, cacheManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, v.size());
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 7);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 100);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 999);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
}

TEST(FileReader, ReadPageSizedFile)
{
    std::vector<uint8_t> v = makeRandomVector(5 * 4096);
    auto cacheManager = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    FileDescriptor fd = writeFile(v, cacheManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, v.size());
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 97);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 256);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4095);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4096);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4097);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
}

TEST(FileReader, ReadBigFile)
{
    std::vector<uint8_t> v = makeRandomVector(17 * 4097);
    auto cacheManager = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    FileDescriptor fd = writeFile(v, cacheManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, v.size());
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 97);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 256);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4095);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4096);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4097);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
}

TEST(FileReader, ReadFragmentedFile)
{
    std::vector<uint8_t> v = makeVector(2200 * 4097); // => 3 filetable pages
    auto cacheManager = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    FileDescriptor fd = writeFragmentedFile(v, cacheManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, v.size());
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 97);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 256);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4095);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4096);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4097);
        ASSERT_EQ(end , (&res[0] + res.size()));
        ASSERT_EQ(res , v);
    }
}
