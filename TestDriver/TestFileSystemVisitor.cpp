

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
};

}

TEST(FileSystemVisitor, NonExistantMakesNoVisitation)
{
    auto fs = makeFileSystem();

    FileSystemVisitor fsvisitor(fs);
    TestVisitor visitor;
    fsvisitor.visit("nonExistant", visitor);
    ASSERT_TRUE(visitor.m_names.empty());

    createFile("folder/file1", fs);
    createFile("folder/file2", fs);
    createFile("folder/subFolder/file3", fs);

    fsvisitor.visit("nonExistant", visitor);
    ASSERT_TRUE(visitor.m_names.empty());
}

TEST(FileSystemVisitor, VisitSingleItem)
{
    auto fs = makeFileSystem();

    FileSystemVisitor fsvisitor(fs);

    createFile("file0", fs);
    TestVisitor visitor0;
    fsvisitor.visit("file0", visitor0);
    ASSERT_EQ(visitor0.m_names[0], "file0");

    createFile("folder/file1", fs);
    createFile("folder/file2", fs);
    createFile("folder/subFolder/file3", fs);

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
    auto fs = makeFileSystem();

    FileSystemVisitor fsvisitor(fs);

    createFile("file0", fs);
    createFile("folder/file1", fs);
    createFile("folder/file2", fs);
    createFile("folder/subFolder/file3", fs);

    TestVisitor visitor0;
    fsvisitor.visit("", visitor0);
    ASSERT_EQ(visitor0.m_names.size(), 7);
}

TEST(FileSystemVisitor, VisitSubFolder)
{
    auto fs = makeFileSystem();

    FileSystemVisitor fsvisitor(fs);

    createFile("file0", fs);
    createFile("folder/file1", fs);
    createFile("folder/file2", fs);
    createFile("folder/subFolder/file3", fs);

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


TEST(FsCompareVisitor, EmptyRootsAreEqual)
{
    auto fs = makeFileSystem();
    FileSystemVisitor fsvisitor(fs);
    auto fs2 = makeFileSystem();

    FsCompareVisitor fscv(fs, fs2, "");
    fsvisitor.visit("", fscv);
    ASSERT_EQ(fscv.m_result, FsCompareVisitor::Result::Equal);
}

TEST(FsCompareVisitor, EmptyFoldersAreEqual)
{
    auto fs = makeFileSystem();
    fs.createPath(Path("folder1"));

    auto fs2 = makeFileSystem();
    fs2.createPath(Path("folder2"));

    FsCompareVisitor fscv(fs, fs2, "folder2");
    FileSystemVisitor fsvisitor(fs);
    fsvisitor.visit("folder1", fscv);
    ASSERT_EQ(fscv.m_result, FsCompareVisitor::Result::Equal);
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
    ASSERT_EQ(fscv.m_result, FsCompareVisitor::Result::Equal);
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
    ASSERT_EQ(fscv.m_result, FsCompareVisitor::Result::Equal);
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
    ASSERT_EQ(fscv.m_result, FsCompareVisitor::Result::Equal);
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
    ASSERT_NE(fscv.m_result, FsCompareVisitor::Result::Equal);
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
    ASSERT_EQ(fscv.m_result, FsCompareVisitor::Result::Equal);

    FsCompareVisitor fscv2(fs, fs2, "");
    FileSystemVisitor fsvisitor2(fs);
    fsvisitor2.visit("", fscv2);
    ASSERT_EQ(fscv2.m_result, FsCompareVisitor::Result::Equal);
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
    ASSERT_EQ(fscv.m_result, FsCompareVisitor::Result::NotEqual);

    FsCompareVisitor fscv2(fs, fs2, "");
    FileSystemVisitor fsvisitor2(fs);
    fsvisitor2.visit("", fscv2);
    ASSERT_EQ(fscv2.m_result, FsCompareVisitor::Result::NotEqual);
}
