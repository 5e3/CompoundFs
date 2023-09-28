

#include <gtest/gtest.h>
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/Path.h"
#include "CompoundFs/DirectoryStructure.h"

using namespace TxFs;

namespace
{

    DirectoryStructure makeDirectoryStructure()
    {
        auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
        auto startup = DirectoryStructure::initialize(cm);
        return DirectoryStructure(startup);
    }

}

TEST(Path, noSubFolderDoesNotChangePath)
{
    auto ds = makeDirectoryStructure();
    Path p("test");
    Path p2 = p;

    ASSERT_TRUE(p.create(&ds));
    ASSERT_EQ(p.m_parent , Path::RootFolder);
    ASSERT_EQ(p.m_relativePath , "test");
    ASSERT_EQ(p , p2);
}

TEST(Path, createCreatesSubFolders)
{
    auto ds = makeDirectoryStructure();
    Path p("folder/folder2");
    Path p2 = p;

    ASSERT_TRUE(p.create(&ds));
    ASSERT_NE(p.m_parent , Path::RootFolder);
    ASSERT_EQ(p.m_relativePath , "folder2");
    ASSERT_TRUE(ds.subFolder(DirectoryKey("folder")));
    ASSERT_NE(p , p2);
}

TEST(Path, creationOfFolderFailsWhenTheNameIsUsedForAnAttribute)
{
    auto ds = makeDirectoryStructure();
    Path p("folder/folder/attribute");

    ASSERT_TRUE(p.create(&ds));
    ds.addAttribute(DirectoryKey(p.m_parent, p.m_relativePath), 1.1);
    Path p2("folder/folder/attribute/test");
    Path p3 = p2;
    ASSERT_TRUE(!p2.create(&ds));
    ASSERT_EQ(p2 , p3);

    ds.remove(DirectoryKey(p.m_parent, p.m_relativePath));
    ASSERT_TRUE(p2.create(&ds));
    ASSERT_NE(p2 , p3);
}

TEST(Path, reduceFindsSubFolders)
{
    auto ds = makeDirectoryStructure();
    Path p("folder/folder/folder/folder/file.file");
    Path p2 = p;
    Path p3 = p;

    ASSERT_TRUE(p.create(&ds));
    ASSERT_TRUE(p2.create(&ds));
    ASSERT_TRUE(p3.normalize(&ds));

    ASSERT_EQ(p , p2);
    ASSERT_EQ(p , p3);
    ASSERT_NE(p.m_parent , Path::RootFolder);
    ASSERT_EQ(p.m_relativePath , "file.file");
}

