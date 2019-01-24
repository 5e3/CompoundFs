#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/File.h"
#include "../CompoundFs/Blob.h"
#include <string>

using namespace TxFs;

TEST(RawFileWriter, CtorCreatesEmptyFileDesc)
{
    RawFileWriter f(std::shared_ptr<PageManager>(new PageManager));
    FileDescriptor fd = f.close();
    CHECK(fd == FileDescriptor());

    f.write(0, 0);
    fd = f.close();
    CHECK(fd == FileDescriptor());
}

TEST(RawFileWriter, WriteCloseCreatesFileDescriptor)
{
    RawFileWriter f(std::shared_ptr<PageManager>(new PageManager));
    Blob data("Test");
    f.write(data.begin(), data.end());
    FileDescriptor fd = f.close();

    CHECK(fd != FileDescriptor());
    CHECK(fd.m_fileSize == data.size());
    CHECK(fd.m_first == fd.m_last);
}

TEST(RawFileWriter, WriteCloseCreatesFileTablePage)
{
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
    Blob data("Test");
    f.write(data.begin(), data.end());
    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);

    auto fileTablePage = pageManager->loadFileTable(fd.m_last);
    CHECK(fileTablePage.second == fd.m_last);

    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    CHECK(is.size() == 1);
    CHECK(is.front() == Interval(0, 1));
}

TEST(RawFileWriter, MultipleWritesCreatesDescOfAppropriateSize)
{
    RawFileWriter f(std::shared_ptr<PageManager>(new PageManager));
    Blob data("Test");

    for (int i = 0; i < 10; i++)
        f.write(data.begin(), data.end());

    FileDescriptor fd = f.close();

    CHECK(fd != FileDescriptor());
    CHECK(fd.m_fileSize == 10ULL * data.size());
    CHECK(fd.m_first == fd.m_last);
}

TEST(RawFileWriter, MultipleOpenCreatesDescOfAppropriateSize)
{
    RawFileWriter f(std::shared_ptr<PageManager>(new PageManager));
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

TEST(RawFileWriter, SmallWritesOverPageBoundery)
{
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
    Blob data("Test");
    for (int i = 0; i < 1000; i++)
        f.write(data.begin(), data.end());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 1000ULL * data.size());

    auto fileTablePage = pageManager->loadFileTable(fd.m_last);
    CHECK(fileTablePage.second == fd.m_last);

    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    CHECK(is.size() == 1);
    CHECK(is.front() == Interval(0, 2));
}

TEST(RawFileWriter, SmallWritesWithAppendOverPageBoundery)
{
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
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

    auto fileTablePage = pageManager->loadFileTable(fd.m_last);
    CHECK(fileTablePage.second == fd.m_last);

    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    CHECK(is.size() == 2);
    CHECK(is.front() == Interval(0, 2));
    CHECK(is.back() == Interval(3, 4));
}

TEST(RawFileWriter, PageSizeWrites)
{
    std::string str(4096, 'X');
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = pageManager->loadFileTable(fd.m_last);
    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    CHECK(is.front() == Interval(0, 10));
}

TEST(RawFileWriter, OverPageSizeWrites)
{
    std::string str(4097, 'X');
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = pageManager->loadFileTable(fd.m_last);
    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    CHECK(is.front() == Interval(0, 11));
}

TEST(RawFileWriter, UnderPageSizeWrites)
{
    std::string str(4095, 'X');
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = pageManager->loadFileTable(fd.m_last);
    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    CHECK(is.front() == Interval(0, 10));
}

TEST(RawFileWriter, LargePageSizeWrites)
{
    std::string str(8192, 'X');
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = pageManager->loadFileTable(fd.m_last);
    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    CHECK(is.front() == Interval(0, 20));
}

TEST(RawFileWriter, LargeSizeWrites)
{
    std::string str(20000, 'X');
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
    for (int i = 0; i < 10; i++)
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());

    FileDescriptor fd = f.close();
    CHECK(fd.m_first == fd.m_last);
    CHECK(fd.m_fileSize == 10ULL * str.size());

    auto fileTablePage = pageManager->loadFileTable(fd.m_last);
    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    CHECK(is.front() == Interval(0, uint32_t(10 * str.size() / 4096 + 1)));
}

TEST(RawFileWriter, LargeSizeWritesMultiFiles)
{
    std::string str(20000, 'X');
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
    RawFileWriter f2(pageManager);
    for (int i = 0; i < 10; i++)
    {
        f.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());
        f2.write((const uint8_t*) str.c_str(), (const uint8_t*) str.c_str() + str.size());
    }

    FileDescriptor fd = f.close();
    FileDescriptor fd2 = f2.close();
    CHECK(fd.m_fileSize == 10ULL * str.size());
    CHECK(fd2.m_fileSize == 10ULL * str.size());

    auto fileTablePage = pageManager->loadFileTable(fd.m_last);
    auto fileTablePage2 = pageManager->loadFileTable(fd2.m_last);
    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    CHECK(is.front() == Interval(0, uint32_t(str.size() / 4096 + 1)));

    IntervalSequence is2;
    fileTablePage2.first->insertInto(is2);
    CHECK(is2.totalLength() == 10 * str.size() / 4096 + 1);
}

TEST(RawFileWriter, FillPageTable)
{
    std::string str(4096, 'X');
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileWriter f(pageManager);
    RawFileWriter f2(pageManager);
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

    auto fileTablePage = pageManager->loadFileTable(fd.m_first);
    IntervalSequence is;
    fileTablePage.first->insertInto(is);
    fileTablePage = pageManager->loadFileTable(fd.m_last);
    fileTablePage.first->insertInto(is);
    CHECK(is.size() == 2000);

    fileTablePage = pageManager->loadFileTable(fd2.m_first);
    fileTablePage.first->insertInto(is);
    fileTablePage = pageManager->loadFileTable(fd2.m_last);
    fileTablePage.first->insertInto(is);
    CHECK(is.size() == 4000);
}

TEST(RawPageReader, ReadNullFile)
{
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    RawFileReader f(pageManager);

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

FileDescriptor writeFragmentedFile(const std::vector<uint8_t>& v, std::shared_ptr<PageManager> pageManager)
{
    RawFileWriter fr(pageManager);
    RawFileWriter fr2(pageManager);
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
    for (auto& elem : v)
    {
        elem = i++;
        if (i >= 251)
            i = 0;
    }
    return v;
}

FileDescriptor writeFile(const std::vector<uint8_t>& v, std::shared_ptr<PageManager> pageManager)
{
    RawFileWriter fr(pageManager);
    fr.writeIterator(v.begin(), v.end());
    return fr.close();
}

uint8_t* readFile(FileDescriptor fd, std::vector<uint8_t>& v, std::shared_ptr<PageManager> pageManager, size_t sizes)
{
    RawFileReader fr(pageManager);
    fr.open(fd);
    uint8_t* begin = &v[0];
    while (fr.bytesLeft())
    {
        begin = fr.read(begin, begin + sizes);
    }
    return begin;
}

TEST(RawFileReader, ReadSmallFile)
{
    std::vector<uint8_t> v = makeRandomVector(1000);
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    FileDescriptor fd = writeFile(v, pageManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, v.size());
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 7);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 100);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 999);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
}

TEST(RawFileReader, ReadPageSizedFile)
{
    std::vector<uint8_t> v = makeRandomVector(5 * 4096);
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    FileDescriptor fd = writeFile(v, pageManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, v.size());
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 97);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 256);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 4095);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 4096);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 4097);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
}

TEST(RawFileReader, ReadBigFile)
{
    std::vector<uint8_t> v = makeRandomVector(17 * 4097);
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    FileDescriptor fd = writeFile(v, pageManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, v.size());
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 97);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 256);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 4095);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 4096);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 4097);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
}

TEST(RawFileReader, ReadFragmentedFile)
{
    std::vector<uint8_t> v = makeVector(2200 * 4097); // => 3 filetable pages
    auto pageManager = std::shared_ptr<PageManager>(new PageManager);
    FileDescriptor fd = writeFragmentedFile(v, pageManager);
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, v.size());
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 97);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 256);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 4095);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 4096);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
    {
        std::vector<uint8_t> res(v.size());
        uint8_t* end = readFile(fd, res, pageManager, 4097);
        CHECK(end == (&res[0] + res.size()));
        CHECK(res == v);
    }
}
