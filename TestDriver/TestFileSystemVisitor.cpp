

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

void createFile(Path path, FileSystem& fs)
{
    auto fh = *fs.createFile(path);
    ByteStringView data("test");
    fs.write(fh, data.data(), data.size());
    fs.close(fh);
}

struct TestVisitor
{
    std::vector<std::string> m_names;
    void operator()(Path p, const TreeValue&) 
    { 
        m_names.push_back(std::string(p.m_relativePath)); 
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
