#include "stdafx.h"
#include "Test.h"
#include "SimpleFile.h"

#include "../CompoundFs/FileReader.h"
#include "../CompoundFs/FileWriter.h"
#include "../CompoundFs/Blob.h"
#include <string>

using namespace TxFs;

TEST(FileWriter, CtorCreatesEmptyFileDesc)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);

    FileWriter f(cm);
    FileDescriptor fd = f.close();
    CHECK(fd == FileDescriptor());

    f.write(0, 0);
    fd = f.close();
    CHECK(fd == FileDescriptor());
}

TEST(FileWriter, WriteCloseCreatesFileDescriptor)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);

    FileWriter f(cm);
    Blob data("Test");
    f.write(data.begin(), data.end());
    FileDescriptor fd = f.close();

    CHECK(fd != FileDescriptor());
    CHECK(fd.m_fileSize == data.size());
    CHECK(fd.m_first == fd.m_last);
}

TEST(FileWriter, WriteCloseCreatesFileTablePage)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    Blob data("Test");
    f.write(data.begin(), data.end());
    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    CHECK(fileTablePage.m_index == fd.m_last);

    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    CHECK(is.size() == 1);
    CHECK(is.front() == Interval(0, 1));
}

TEST(FileWriter, MultipleWritesCreatesDescOfAppropriateSize)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);

    FileWriter f(cm);
    Blob data("Test");

    for (int i = 0; i < 10; i++)
        f.write(data.begin(), data.end());

    FileDescriptor fd = f.close();

    CHECK(fd != FileDescriptor());
    CHECK(fd.m_fileSize == 10ULL * data.size());
    CHECK(fd.m_first == fd.m_last);
}

TEST(FileWriter, MultipleOpenCreatesDescOfAppropriateSize)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);

    FileWriter f(cm);
    Blob data("Test");

    for (int i = 0; i < 10; i++)
        f.write(data.begin(), data.end());

    FileDescriptor fd = f.close();

    f.openAppend(fd);
    for (int i = 0; i < 10; i++)
        f.write(data.begin(), data.end());

    fd = f.close();

    CHECK(fd != FileDescriptor());
    CHECK(fd.m_fileSize == 20ULL * data.size());
    CHECK(fd.m_first == fd.m_last);
}

TEST(FileWriter, SmallWritesOverPageBoundery)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    Blob data("Test");
    for (int i = 0; i < 1000; i++)
        f.write(data.begin(), data.end());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 1000ULL * data.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    CHECK(fileTablePage.m_index == fd.m_last);

    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    CHECK(is.size() == 1);
    CHECK(is.front() == Interval(0, 2));
}

TEST(FileWriter, SmallWritesWithAppendOverPageBoundery)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    Blob data("Test");
    for (int i = 0; i < 1000; i++)
        f.write(data.begin(), data.end());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 1000ULL * data.size());

    f.openAppend(fd);
    for (int i = 0; i < 1000; i++)
        f.write(data.begin(), data.end());

    fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 2000ULL * data.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    CHECK(fileTablePage.m_index == fd.m_last);

    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    CHECK(is.size() == 2);
    CHECK(is.front() == Interval(0, 2));
    CHECK(is.back() == Interval(3, 4));
}

TEST(FileWriter, PageSizeWrites)
{
    std::string str(4096, 'X');
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    CHECK(is.front() == Interval(0, 10));
}

TEST(FileWriter, OverPageSizeWrites)
{
    std::string str(4097, 'X');
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    CHECK(is.front() == Interval(0, 11));
}

TEST(FileWriter, UnderPageSizeWrites)
{
    std::string str(4095, 'X');
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    CHECK(is.front() == Interval(0, 10));
}

TEST(FileWriter, LargePageSizeWrites)
{
    std::string str(8192, 'X');
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    CHECK(is.front() == Interval(0, 20));
}

TEST(FileWriter, LargeSizeWrites)
{
    std::string str(20000, 'X');
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);

    FileWriter f(cm);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    CHECK(is.front() == Interval(0, uint32_t(10 * str.size() / 4096 + 1)));
}

TEST(FileWriter, LargeSizeWritesMultiFiles)
{
    std::string str(20000, 'X');
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
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
    CHECK(fd.m_fileSize == 10ULL * str.size());
    CHECK(fd2.m_fileSize == 10ULL * str.size());

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    auto fileTablePage2 = tcm.loadPage<FileTable>(fd2.m_last);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    CHECK(is.front() == Interval(0, uint32_t(str.size() / 4096 + 1)));

    IntervalSequence is2;
    fileTablePage2.m_page->insertInto(is2);
    CHECK(is2.totalLength() == 10 * str.size() / 4096 + 1);
}

TEST(FileWriter, FillPageTable)
{
    std::string str(4096, 'X');
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
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
    CHECK(fd.m_fileSize == 2000ULL * str.size());
    CHECK(fd.m_first != fd.m_last);

    CHECK(fd2.m_fileSize == 2000ULL * str.size());
    CHECK(fd2.m_first != fd2.m_last);

    auto fileTablePage = tcm.loadPage<FileTable>(fd.m_first);
    IntervalSequence is;
    fileTablePage.m_page->insertInto(is);
    fileTablePage = tcm.loadPage<FileTable>(fd.m_last);
    fileTablePage.m_page->insertInto(is);
    CHECK(is.size() == 2000);

    fileTablePage = tcm.loadPage<FileTable>(fd2.m_first);
    fileTablePage.m_page->insertInto(is);
    fileTablePage = tcm.loadPage<FileTable>(fd2.m_last);
    fileTablePage.m_page->insertInto(is);
    CHECK(is.size() == 4000);
}

TEST(RawPageReader, ReadNullFile)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    FileReader f(cm);

    CHECK(f.read(0, 0) == 0);

    uint8_t buf;
    CHECK(f.read(&buf, &buf + 1) == &buf);
    CHECK(f.bytesLeft() == 0);
}

std::vector<uint8_t> makeRandomVector(size_t size)
{
    std::vector<uint8_t> v(size);
    for (size_t i = 0; i < size; i++)
        v[i] = uint8_t(i);
    std::random_shuffle(v.begin(), v.end());
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
    SimpleFile sf;
    auto cacheManager = std::make_shared<CacheManager>(&sf);
    FileDescriptor fd = writeFile(v, cacheManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, v.size());
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 7);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 100);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 999);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
}

TEST(FileReader, ReadPageSizedFile)
{
    std::vector<uint8_t> v = makeRandomVector(5 * 4096);
    SimpleFile sf;
    auto cacheManager = std::make_shared<CacheManager>(&sf);
    FileDescriptor fd = writeFile(v, cacheManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, v.size());
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 97);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 256);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4095);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4096);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4097);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
}

TEST(FileReader, ReadBigFile)
{
    std::vector<uint8_t> v = makeRandomVector(17 * 4097);
    SimpleFile sf;
    auto cacheManager = std::make_shared<CacheManager>(&sf);
    FileDescriptor fd = writeFile(v, cacheManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, v.size());
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 97);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 256);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4095);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4096);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4097);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
}

TEST(FileReader, ReadFragmentedFile)
{
    std::vector<uint8_t> v = makeVector(2200 * 4097); // => 3 filetable pages
    SimpleFile sf;
    auto cacheManager = std::make_shared<CacheManager>(&sf);
    FileDescriptor fd = writeFragmentedFile(v, cacheManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, v.size());
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 97);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 256);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4095);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4096);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, cacheManager, 4097);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
}
