


#include <gtest/gtest.h>
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/DirectoryStructure.h"
#include "CompoundFs/CommitBlock.h"

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

TEST(DirectoryStructure, FoldersAreNotFound)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto res = ds.subFolder(DirectoryKey("test"));
    ASSERT_TRUE(!res);
}

TEST(DirectoryStructure, makeFolderReturnsFolder)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto res = ds.makeSubFolder(DirectoryKey("test"));
    ASSERT_TRUE(res);
    auto folder = *res;

    res = ds.makeSubFolder(DirectoryKey("test"));
    ASSERT_TRUE(res);
    ASSERT_EQ(*res, folder);

    ASSERT_EQ(ds.subFolder(DirectoryKey("test")), *res);
}

TEST(DirectoryStructure, makeRootFolder)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto res = ds.makeSubFolder(DirectoryKey(""));
    ASSERT_TRUE(res);
    ASSERT_EQ(*res, DirectoryKey::RootFolder);
}

TEST(DirectoryStructure, subFolderLookupRootFolder)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto res = ds.subFolder(DirectoryKey(""));
    ASSERT_TRUE(res);
    ASSERT_EQ(*res, DirectoryKey::RootFolder);
}

TEST(DirectoryStructure, makeSubFolder)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder"));
    auto subsub = ds.makeSubFolder(DirectoryKey(*subFolder, "subsub"));
    ASSERT_TRUE(subsub);
    ASSERT_EQ(subsub , ds.subFolder(DirectoryKey(*subFolder, "subsub")));
}

TEST(DirectoryStructure, simpleRemove)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder")).value();
    auto subsub = ds.makeSubFolder(DirectoryKey(subFolder, "subsub"));
    DirectoryKey dkey(subFolder, "subsub");
    auto nof = ds.remove(dkey);
    ASSERT_EQ(nof , 1);
    ASSERT_TRUE(!ds.subFolder(DirectoryKey(subFolder, "subsub")));
}

TEST(DirectoryStructure, recursiveRemove)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder")).value();
    ds.makeSubFolder(DirectoryKey(subFolder, "subsub1"));
    ds.makeSubFolder(DirectoryKey(subFolder, "subsub2"));
    ds.makeSubFolder(DirectoryKey(subFolder, "subsub3"));
    ds.makeSubFolder(DirectoryKey(subFolder, "subsub4"));
    DirectoryKey dkey("subFolder");
    auto nof = ds.remove(dkey);
    ASSERT_EQ(nof , 5);
}

TEST(DirectoryStructure, recursiveRemove2)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder")).value();
    auto subFolder2 = ds.makeSubFolder(DirectoryKey("subFolder2")).value();
    ds.makeSubFolder(DirectoryKey(subFolder2, "subsub1"));

    ds.makeSubFolder(DirectoryKey(subFolder, "subsub1"));
    ds.makeSubFolder(DirectoryKey(subFolder, "subsub2"));
    ds.makeSubFolder(DirectoryKey(subFolder, "subsub3"));
    ds.makeSubFolder(DirectoryKey(subFolder, "subsub4"));
    ds.addAttribute(DirectoryKey(subFolder, "attrib"), "test");
    DirectoryKey dkey("subFolder");
    auto nof = ds.remove(dkey);
    ASSERT_EQ(nof, 6);
    ASSERT_TRUE(!ds.subFolder(DirectoryKey(subFolder, "subsub1")));
    ASSERT_TRUE(!ds.subFolder(DirectoryKey(subFolder, "subsub2")));
    ASSERT_TRUE(!ds.getAttribute(DirectoryKey(subFolder, "attrib")));
    ASSERT_TRUE(ds.subFolder(DirectoryKey(subFolder2, "subsub1")));
}

TEST(DirectoryStructure, recursiveRemoveReturnsNumOfDeletedItems)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder")).value();
    for (int i = 0; i < 4; i++)
    {
        auto subSubFolder = ds.makeSubFolder(DirectoryKey(subFolder, std::to_string(i))).value();
        for (int j = 0; j < 3; j++)
            ds.addAttribute(DirectoryKey(subSubFolder, std::to_string(j)), "test");
    }
    DirectoryKey dkey("subFolder");
    auto nof = ds.remove(dkey);
    ASSERT_EQ(nof, 1+4+4*3);
}

TEST(DirectoryStructure, addGetAttribute)
{
    DirectoryStructure ds = makeDirectoryStructure();

    ASSERT_TRUE(ds.addAttribute(DirectoryKey("attrib"), "test"));
    auto res = ds.getAttribute(DirectoryKey("attrib"));
    ASSERT_TRUE(res);
    ASSERT_EQ(res->toValue<std::string>(), "test");

    ASSERT_TRUE(ds.addAttribute(DirectoryKey("attrib"), 42.42));
    res = ds.getAttribute(DirectoryKey("attrib"));
    ASSERT_TRUE(res);
    ASSERT_EQ(res->toValue<double>(), 42.42);
}

TEST(DirectoryStructure, attributesDoNotReplaceFolders)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder")).value();
    ASSERT_TRUE(!ds.addAttribute(DirectoryKey("subFolder"), "test"));
    auto res = ds.getAttribute(DirectoryKey("subFolder"));
    ASSERT_TRUE(!res);
}

TEST(DirectoryStructure, foldersDoNotReplaceAttributes)
{
    DirectoryStructure ds = makeDirectoryStructure();

    ASSERT_TRUE(ds.addAttribute(DirectoryKey("subFolder"), "test"));
    ASSERT_TRUE(!ds.makeSubFolder(DirectoryKey("subFolder")));
}

TEST(DirectoryStructure, createFile)
{
    DirectoryStructure ds = makeDirectoryStructure();
    DirectoryKey dkey("test.file");

    ASSERT_TRUE(ds.createFile(dkey));
    FileDescriptor desc(100);
    ASSERT_TRUE(ds.updateFile(dkey, desc));

    ASSERT_EQ(*ds.openFile(dkey) , desc);
    ASSERT_TRUE(ds.createFile(dkey));
    ASSERT_EQ(*ds.openFile(dkey) , FileDescriptor());
}

TEST(DirectoryStructure, appendFile)
{
    DirectoryStructure ds = makeDirectoryStructure();
    DirectoryKey dkey("test.file");

    ASSERT_EQ(*ds.appendFile(dkey) , FileDescriptor());

    FileDescriptor desc(100);
    ASSERT_TRUE(ds.updateFile(dkey, desc));

    ASSERT_EQ(*ds.appendFile(dkey) , desc);
}
                                           
TEST(DirectoryStructure, updateFileNeedsExistingFile)
{
    DirectoryStructure ds = makeDirectoryStructure();
    DirectoryKey dkey("test.file");

    FileDescriptor desc(100);
    ASSERT_TRUE(!ds.updateFile(dkey, desc));
}

TEST(DirectoryStructure, nonFileEntryPreventsCreationOfFile)
{
    DirectoryStructure ds = makeDirectoryStructure();
    DirectoryKey dkey("test.file");
    ds.addAttribute(dkey, 1.1);

    ASSERT_TRUE(!ds.createFile(dkey));
    ASSERT_TRUE(!ds.appendFile(dkey));
    FileDescriptor desc(100);
    ASSERT_TRUE(!ds.updateFile(dkey, desc));
}

TEST(DirectoryStructure, storeCommitBlockEqualsRetrieveCommitBlock)
{
    auto ds = makeDirectoryStructure();
    CommitBlock out { { 1234567, 123, 234 }, 54321, 23 };
    ds.storeCommitBlock(out);
    auto in = ds.retrieveCommitBlock();
    ASSERT_EQ(in.m_freeStoreDescriptor, out.m_freeStoreDescriptor);
    ASSERT_EQ(in.m_compositSize, out.m_compositSize);
    ASSERT_EQ(in.m_maxFolderId, out.m_maxFolderId);
}

TEST(DirectoryStructure, EmptyFolderReturnsNullCursorOnBegin)
{
    auto ds = makeDirectoryStructure();
    auto folder1 = *ds.makeSubFolder(DirectoryKey("test1"));
    auto folder2 = *ds.makeSubFolder(DirectoryKey("test2"));
    ds.addAttribute(DirectoryKey(folder2, "testAttrib"), "test");

    auto cursor = ds.begin(DirectoryKey(folder1, ""));
    ASSERT_FALSE(cursor);
}


TEST(Cursor, creation)
{
    DirectoryStructure::Cursor cursor;
    auto cur2 = cursor;
    ASSERT_EQ(cur2 , cursor);
    ASSERT_TRUE(!cursor);

    DirectoryStructure ds = makeDirectoryStructure();
    ASSERT_TRUE(ds.addAttribute(DirectoryKey("attrib"), "test"));
    auto cur3 = ds.find(DirectoryKey("attrib"));

    ASSERT_TRUE(cur3);
    ASSERT_NE(cur3 , cur2);
    auto res = cur3.key();
    ASSERT_EQ(res.first , DirectoryKey::RootFolder);
    ASSERT_EQ(res.second , "attrib");
    auto attrib = cur3.value();
    ASSERT_EQ(attrib.toValue<std::string>() , "test");
    ASSERT_EQ(attrib.getType(), TreeValue::Type::String);
    ASSERT_EQ(attrib.getTypeName(), "String");

    auto cur4 = ds.next(cur3);
    ASSERT_TRUE(!cur4);
}

namespace
{
    Folder makeFilledSubFolder(DirectoryStructure& ds, Folder folder, std::string_view name)
    {
        auto newFolder = ds.makeSubFolder(DirectoryKey(folder, name)).value();
        for (uint64_t i = 0; i < 50; i++)
            ds.addAttribute(DirectoryKey(newFolder, std::to_string(i) + name.data()), i);
        return newFolder;
    }
}

TEST(Cursor, iteratesOverFolder)
{
    auto ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder")).value();
    ds.addAttribute(DirectoryKey("attrib"), "test");
    makeFilledSubFolder(ds, subFolder, "Test1");
    auto folder = makeFilledSubFolder(ds, subFolder, "Test2");
    makeFilledSubFolder(ds, subFolder, "Test3");

    auto cur = ds.begin(DirectoryKey(folder));
    ASSERT_EQ(cur.key().second, "0Test2");
    ASSERT_EQ(cur.value().toValue<uint64_t>(), 0ULL);

    std::set<uint64_t> iset;
    for (; cur; cur = ds.next(cur))
    {
        ASSERT_EQ(cur.key().first, folder);
        iset.insert(cur.value().toValue<uint64_t>());
    }
    ASSERT_EQ(iset.size(), 50);
}