


#include <gtest/gtest.h>
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/DirectoryStructure.h"

using namespace TxFs;

namespace
{

DirectoryStructure makeDirectoryStructure()
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();
    FileDescriptor fsfd(freeStorePage.m_index);
    return DirectoryStructure(cm, fsfd);
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
    ASSERT_EQ(*res , Folder { 1 });

    res = ds.makeSubFolder(DirectoryKey("test"));
    ASSERT_TRUE(res);
    ASSERT_EQ(*res , Folder { 1 });

    ASSERT_EQ(ds.subFolder(DirectoryKey("test")) , *res);
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
    ASSERT_EQ(nof , 6);
    ASSERT_TRUE(!ds.subFolder(DirectoryKey(subFolder, "subsub1")));
    ASSERT_TRUE(!ds.subFolder(DirectoryKey(subFolder, "subsub2")));
    ASSERT_TRUE(!ds.getAttribute(DirectoryKey(subFolder, "attrib")));
    ASSERT_TRUE(ds.subFolder(DirectoryKey(subFolder2, "subsub1")));
}

TEST(DirectoryStructure, addGetAttribute)
{
    DirectoryStructure ds = makeDirectoryStructure();

    ASSERT_TRUE(ds.addAttribute(DirectoryKey("attrib"), "test"));
    auto res = ds.getAttribute(DirectoryKey("attrib"));
    ASSERT_TRUE(res);
    ASSERT_EQ(std::get<std::string>(*res) , "test");

    ASSERT_TRUE(ds.addAttribute(DirectoryKey("attrib"), 42));
    res = ds.getAttribute(DirectoryKey("attrib"));
    ASSERT_TRUE(res);
    ASSERT_EQ(std::get<int>(*res) , 42);
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
    ds.addAttribute(dkey, 1);

    ASSERT_TRUE(!ds.createFile(dkey));
    ASSERT_TRUE(!ds.appendFile(dkey));
    FileDescriptor desc(100);
    ASSERT_TRUE(!ds.updateFile(dkey, desc));
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
    ASSERT_EQ(res.first , DirectoryKey::Root);
    ASSERT_EQ(res.second , "attrib");
    ASSERT_EQ(std::get<std::string>(cur3.value()) , "test");
    ASSERT_EQ(cur3.getValueType() , DirectoryObjType::String);
    ASSERT_EQ(cur3.getValueTypeName() , "String");

    auto cur4 = ds.next(cur3);
    ASSERT_TRUE(!cur4);
}
