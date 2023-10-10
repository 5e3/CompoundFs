

#include <gtest/gtest.h>
#include "FileSystemUtility.h"
#include "CompoundFs/FileSystemVisitor.h"
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/DirectoryStructure.h"
#include "CompoundFs/Path.h"
#include "CompoundFs/FileSystemHelper.h"
#include <string>

using namespace std::string_literals;
using namespace TxFs;

namespace
{

FileSystem makeFileSystem()
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    auto fs = FileSystem(FileSystem::initialize(cm));
    fs.commit();
    return fs;
}

void createFile(Path path, FileSystem& fs, std::string data = "test")
{
    auto fh = *fs.createFile(path);
    fs.write(fh, data.data(), data.size());
    fs.close(fh);
}

struct TestVisitor
{
    std::vector<std::string> m_names;
    VisitorControl operator()(Path p, const TreeValue&) 
    { 
        m_names.push_back(std::string(p.m_relativePath));
        return VisitorControl::Continue;
    }

    template <typename TIterator = std::nullptr_t>
    void done(TIterator begin = nullptr, TIterator end = nullptr)
    {
        if constexpr (!std::is_null_pointer_v<TIterator>)
            for (; begin != end; ++begin)
                operator()(begin->m_key, begin->m_value);
    }
};

FileSystem createEnvironment()
{
    auto fs = makeFileSystem();

    FileSystemVisitor fsvisitor(fs);

    createFile("file0", fs);
    createFile("folder/file1", fs);
    createFile("folder/file2", fs);
    createFile("folder/subFolder/file3", fs);
    return fs;
}
}

TEST(FileSystemVisitor, NonExistantMakesNoVisitation)
{
    auto fs = createEnvironment();

    FileSystemVisitor fsvisitor(fs);
    TestVisitor visitor;
    fsvisitor.visit("nonExistant", visitor);
    ASSERT_TRUE(visitor.m_names.empty());
}

TEST(FileSystemVisitor, VisitSingleItem)
{
    auto fs = createEnvironment();

    FileSystemVisitor fsvisitor(fs);

    TestVisitor visitor0;
    fsvisitor.visit("file0", visitor0);
    ASSERT_EQ(visitor0.m_names[0], "file0");

    TestVisitor visitor1;
    fsvisitor.visit("folder/file1", visitor1);
    ASSERT_EQ(visitor1.m_names[0], "file1");

    TestVisitor visitor2;
    fsvisitor.visit("folder/file2", visitor2);
    ASSERT_EQ(visitor2.m_names[0], "file2");

    TestVisitor visitor3;
    fsvisitor.visit("folder/subFolder/file3", visitor3);
    ASSERT_EQ(visitor3.m_names[0], "file3");
}

TEST(FileSystemVisitor, VisitRootFolder)
{
    auto fs = createEnvironment();

    FileSystemVisitor fsvisitor(fs);
    TestVisitor visitor0;
    fsvisitor.visit("", visitor0);
    ASSERT_EQ(visitor0.m_names.size(), 7);
}

TEST(FileSystemVisitor, VisitSubFolder)
{
    auto fs = createEnvironment();

    FileSystemVisitor fsvisitor(fs);

    TestVisitor visitor0;
    fsvisitor.visit("folder", visitor0);
    ASSERT_EQ(visitor0.m_names.size(), 5);

    TestVisitor visitor1;
    fsvisitor.visit("folder/subFolder", visitor1);
    ASSERT_EQ(visitor1.m_names.size(), 2);
}

TEST(FileSystemVisitor, ComplexFs)
{
    auto fs = makeFileSystem();
    createFile("folder1/a.file", fs);
    createFile("folder1/subFolder1/subFolder2/1.file", fs);
    createFile("folder1/x.file", fs);

    TestVisitor visitor0;
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder1", visitor0);
    ASSERT_EQ(visitor0.m_names.size(), 6);

    TestVisitor visitor1;
    fsvisitor.visit("", visitor1);
    ASSERT_EQ(visitor1.m_names.size(), 7);
}

///////////////////////////////////////////////////////////////////////////////

TEST(FsBufferSink, flushesInDefinedPortions)
{
    FsBufferSink<TestVisitor> fbs(5);
    ASSERT_EQ(fbs.getChainedSink().m_names.size(), 0);

    for (int i = 0; i < 7; i++)
        fbs("test", TreeValue());
    ASSERT_EQ(fbs.getChainedSink().m_names.size(), 5);

    for (int i = 0; i < 7; i++)
        fbs("test", TreeValue());
    ASSERT_EQ(fbs.getChainedSink().m_names.size(), 10);

    fbs("test", TreeValue());
    ASSERT_EQ(fbs.getChainedSink().m_names.size(), 15);
}

TEST(FsBufferSink, doneFlushesAll)
{
    FsBufferSink<TestVisitor> fbs(5);
    fbs("test0", TreeValue());
    ASSERT_EQ(fbs.getChainedSink().m_names.size(), 0);

    std::vector<TreeEntry> entries(15, TreeEntry({{ "test1" }, TreeValue() }));
    fbs.done(entries.begin(), entries.end());
    ASSERT_EQ(fbs.getChainedSink().m_names.size(), 16);
    ASSERT_EQ(fbs.getChainedSink().m_names[0], "test0");
    ASSERT_EQ(fbs.getChainedSink().m_names[1], "test1");
}

///////////////////////////////////////////////////////////////////////////////

TEST(FsCompareVisitor, EmptyRootsAreEqual)
{
    auto fs = makeFileSystem();
    FileSystemVisitor fsvisitor(fs);
    auto fs2 = makeFileSystem();

    FsCompareVisitor fscv(fs, fs2, "");
    fsvisitor.visit("", fscv);
    ASSERT_EQ(fscv.result(), FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, EmptyFoldersAreEqual)
{
    auto fs = makeFileSystem();
    fs.makeSubFolder("folder1");

    auto fs2 = makeFileSystem();
    fs2.makeSubFolder("folder2");

    FsCompareVisitor fscv(fs, fs2, "folder2");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder1", fscv);
    ASSERT_EQ(fscv.result(), FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, NonEmptyFoldersAreEqual)
{
    auto fs = makeFileSystem();
    createFile("folder1/file1", fs);

    auto fs2 = makeFileSystem();
    createFile("folder2/file1", fs2);

    FsCompareVisitor fscv(fs, fs2, "folder2");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder1", fscv);
    ASSERT_EQ(fscv.result(), FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, NonEmptySubFoldersAreEqual)
{
    auto fs = makeFileSystem();
    createFile("folder1/subFolder/file1", fs);

    auto fs2 = makeFileSystem();
    createFile("folder2/subFolder/file1", fs2);

    FsCompareVisitor fscv(fs, fs2, "folder2/subFolder");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder1/subFolder", fscv);
    ASSERT_EQ(fscv.result(), FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, SameAttributesReportEqual)
{
    auto fs = makeFileSystem();
    fs.addAttribute("folder1/subFolder/attrib1", "test");
    fs.addAttribute("folder2/subFolder/subFolder2/attrib1", "test");

    FsCompareVisitor fscv(fs, fs, "folder1/subFolder/attrib1");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder2/subFolder/subFolder2/attrib1", fscv);
    ASSERT_EQ(fscv.result(), FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, UnequalAttributeValueReportsNotEqual)
{
    auto fs = makeFileSystem();
    fs.addAttribute("folder1/subFolder/attrib1", "test");
    fs.addAttribute("folder2/subFolder/subFolder2/attrib1", "Test");

    FsCompareVisitor fscv(fs, fs, "folder1/subFolder/attrib1");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder2/subFolder/subFolder2/attrib1", fscv);
    ASSERT_NE(fscv.result(), FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, UnequalAttributeTypeReportsNotEqual)
{
    auto fs = makeFileSystem();
    fs.addAttribute("folder1/subFolder/attrib1", "test");
    fs.addAttribute("folder2/subFolder/subFolder2/attrib1", 1.1);

    FsCompareVisitor fscv(fs, fs, "folder1/subFolder/attrib1");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder2/subFolder/subFolder2/attrib1", fscv);
    ASSERT_NE(fscv.result(), FsCompareVisitor::Result::Equal);

    createFile("folder2/subFolder/subFolder2/file", fs);
    fsvisitor.visit("folder2/subFolder/subFolder2/file", fscv);
    ASSERT_NE(fscv.result(), FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, BigFilesAreEqual)
{
    auto fs = makeFileSystem();
    auto data = std::string(10 * 1000 * 1000, 'x');
    createFile("folder1/file1", fs, data);

    auto fs2 = makeFileSystem();
    createFile("folder2/file1", fs2, data);

    FsCompareVisitor fscv(fs, fs2, "folder2");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder1", fscv);
    ASSERT_EQ(fscv.result(), FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, BigFilesAreNotEqual)
{
    auto fs = makeFileSystem();
    auto data = std::string(10 * 1000 * 1000, 'x');
    createFile("folder1/file1", fs, data);

    auto fs2 = makeFileSystem();
    data.back() = 'y';
    createFile("folder2/file1", fs2, data);

    FsCompareVisitor fscv(fs, fs2, "folder2");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder1", fscv);
    ASSERT_EQ(fscv.result(), FsCompareVisitor::Result::NotEqual);
}

TEST(FsCompareVisitor, ComplexFsIsEqual)
{
    auto fs = makeFileSystem();
    createFile("folder/a.file", fs);
    createFile("folder/subFolder1/subFolder2/1.file", fs);
    createFile("folder/x.file", fs);

    auto fs2 = makeFileSystem();
    createFile("folder/a.file", fs2);
    createFile("folder/subFolder1/subFolder2/1.file", fs2);
    createFile("folder/x.file", fs2);

    FsCompareVisitor fscv(fs, fs2, "folder");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder", fscv);
    ASSERT_EQ(fscv.result(), FsCompareVisitor::Result::Equal);

    FsCompareVisitor fscv2(fs, fs2, "");
    FileSystemVisitor fsvisitor2(fs);
    fsvisitor2.visit("", fscv2);
    ASSERT_EQ(fscv2.result(), FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, ComplexFsIsNotEqual)
{
    auto fs = makeFileSystem();
    createFile("folder/a.file", fs);
    createFile("folder/subFolder1/subFolder2/1.file", fs);
    createFile("folder/x.file", fs);

    auto fs2 = makeFileSystem();
    createFile("folder/a.file", fs2);
    createFile("folder/subFolder1/subFolder2/1.file", fs2, "tesT");
    createFile("folder/x.file", fs2);

    FsCompareVisitor fscv(fs, fs2, "folder");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder", fscv);
    ASSERT_EQ(fscv.result(), FsCompareVisitor::Result::NotEqual);

    FsCompareVisitor fscv2(fs, fs2, "");
    FileSystemVisitor fsvisitor2(fs);
    fsvisitor2.visit("", fscv2);
    ASSERT_EQ(fscv2.result(), FsCompareVisitor::Result::NotEqual);
}

///////////////////////////////////////////////////////////////////////////////

TEST(TempFileBuffer, EmptyFileBufferReturnsEmptyOptional)
{
    Private::TempFileBuffer tfb;

    ASSERT_FALSE(tfb.startReading());
    ASSERT_FALSE(tfb.read());
}

TEST(TempFileBuffer, writeOneReadOneBack)
{
    Private::TempFileBuffer tfb;

    tfb.write("test", "test");
    auto res = tfb.startReading();
    ASSERT_TRUE(res);
    ASSERT_EQ(Path("test") , res->m_key);
    ASSERT_EQ(res->m_value, TreeValue("test"));
}

TEST(TempFileBuffer, writeOneSecondReadIsEmpty)
{
    Private::TempFileBuffer tfb;
    tfb.write("test", "test");
    auto res = tfb.startReading();
    ASSERT_TRUE(res);
    res = tfb.read();
    ASSERT_FALSE(res);
}

TEST(TempFileBuffer, writeManyReadMany)
{
    Private::TempFileBuffer tfb;
    std::vector<TreeEntry> entries;
    for (int i = 0; i < 10; i++)
    {
        auto str = std::to_string(i);
        tfb.write(str.c_str(), str);
        entries.emplace_back(str.c_str() , str );
    }

    std::vector<TreeEntry> entries2;
    for (auto entry = tfb.startReading(); entry; entry = tfb.read())
        entries2.push_back(*entry);

    ASSERT_EQ(entries, entries2);
}

TEST(TempFileBuffer, fileSizeIsBufferSize)
{
    Private::TempFileBuffer tfb;
    std::vector<std::string> entries;

    int i = 1000;
    auto str = std::to_string(i++);
    tfb.write(str.c_str(), std::string(""));
    entries.push_back(str);

    auto minSize = tfb.getFileSize();
    while (tfb.getFileSize() < tfb.getBufferSize() - 100)
    {
        str = std::to_string(i++);
        tfb.write(str.c_str(), "");
        entries.push_back(str);
    }

    str = std::to_string(i++);
    tfb.write(str.c_str(), std::string(tfb.getBufferSize() - tfb.getFileSize() - minSize, ' ').c_str());
    entries.push_back(str);
    ASSERT_EQ(tfb.getFileSize(), tfb.getBufferSize());

    std::vector<std::string> entries2;
    for (auto entry = tfb.startReading(); entry; entry = tfb.read())
        entries2.emplace_back(entry->m_key.getPath().m_relativePath);

    ASSERT_EQ(entries, entries2);
}

TEST(TempFileBuffer, fileSizeIsMoreThanBufferSize)
{
    Private::TempFileBuffer tfb;
    std::vector<std::string> entries;

    int i = 1000;
    auto str = std::to_string(i++);
    tfb.write(str.c_str(), std::string(""));
    entries.push_back(str);

    auto minSize = tfb.getFileSize();
    while (tfb.getFileSize() < 2*tfb.getBufferSize() - 100)
    {
        str = std::to_string(i++);
        tfb.write(str.c_str(), "");
        entries.push_back(str);
    }

    str = std::to_string(i++);
    tfb.write(str.c_str(), std::string(2*tfb.getBufferSize() - tfb.getFileSize() - minSize+1, ' ').c_str());
    entries.push_back(str);
    ASSERT_EQ(tfb.getFileSize(), 2*tfb.getBufferSize()+1);

    std::vector<std::string> entries2;
    auto entry = tfb.startReading();
    while (entry)
    {
        entries2.emplace_back(entry->m_key.getPath().m_relativePath);
        entry = tfb.read();
    }

    i = 0;
    for (auto& e: entries)
        if (e != entries2[i++])
            break;

    ASSERT_EQ(entries, entries2);
}

TEST(TempFileBuffer, fillFileWithMaxSizedObjects)
{
    Private::TempFileBuffer tfb;
    std::vector<TreeEntry> entries;
    int i = 0;
    while (tfb.getFileSize() < 4 * tfb.getBufferSize())
    {
        auto key = std::to_string(i++) + std::string(512, ' ');
        Path path(std::string_view(key.data(), DirectoryKey::maxSize()));
        TreeValue tv(std::string(key.data(), TreeValue::maxVariableSize()));
        entries.emplace_back(path, tv);
        tfb.write(path, tv);
        ASSERT_EQ(tfb.getFileSize() % 512, 0);
    }

    std::vector<TreeEntry> entries2;
    entries2.reserve(entries.size());
    for (auto e = tfb.startReading(); e; e = tfb.read())
        entries2.push_back(*e);

    ASSERT_EQ(entries, entries2);
}
