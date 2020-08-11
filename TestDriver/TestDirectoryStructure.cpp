

#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/DirectoryStructure.h"

using namespace TxFs;

namespace
{

DirectoryStructure makeDirectoryStructure()
{
    auto memFile = new MemoryFile;
    auto cm = std::make_shared<CacheManager>(memFile);
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
    CHECK(!res);
}

TEST(DirectoryStructure, makeFolderReturnsFolder)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto res = ds.makeSubFolder(DirectoryKey("test"));
    CHECK(res);
    CHECK(*res == Folder { 1 });

    res = ds.makeSubFolder(DirectoryKey("test"));
    CHECK(res);
    CHECK(*res == Folder { 1 });

    CHECK(ds.subFolder(DirectoryKey("test")) == *res);
}

TEST(DirectoryStructure, makeSubFolder)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder"));
    auto subsub = ds.makeSubFolder(DirectoryKey(*subFolder, "subsub"));
    CHECK(subsub);
    CHECK(subsub == ds.subFolder(DirectoryKey(*subFolder, "subsub")));
}

TEST(DirectoryStructure, simpleRemove)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder")).value();
    auto subsub = ds.makeSubFolder(DirectoryKey(subFolder, "subsub"));
    DirectoryKey dkey(subFolder, "subsub");
    auto nof = ds.remove(dkey);
    CHECK(nof == 1);
    CHECK(!ds.subFolder(DirectoryKey(subFolder, "subsub")));
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
    CHECK(nof == 5);
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
    CHECK(nof == 6);
    CHECK(!ds.subFolder(DirectoryKey(subFolder, "subsub1")));
    CHECK(!ds.subFolder(DirectoryKey(subFolder, "subsub2")));
    CHECK(!ds.getAttribute(DirectoryKey(subFolder, "attrib")));
    CHECK(ds.subFolder(DirectoryKey(subFolder2, "subsub1")));
}

TEST(DirectoryStructure, addGetAttribute)
{
    DirectoryStructure ds = makeDirectoryStructure();

    CHECK(ds.addAttribute(DirectoryKey("attrib"), "test"));
    auto res = ds.getAttribute(DirectoryKey("attrib"));
    CHECK(res);
    CHECK(std::get<std::string>(*res) == "test");

    CHECK(ds.addAttribute(DirectoryKey("attrib"), 42));
    res = ds.getAttribute(DirectoryKey("attrib"));
    CHECK(res);
    CHECK(std::get<int>(*res) == 42);
}

TEST(DirectoryStructure, attributesDoNotReplaceFolders)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder(DirectoryKey("subFolder")).value();
    CHECK(!ds.addAttribute(DirectoryKey("subFolder"), "test"));
    auto res = ds.getAttribute(DirectoryKey("subFolder"));
    CHECK(!res);
}

TEST(DirectoryStructure, foldersDoNotReplaceAttributes)
{
    DirectoryStructure ds = makeDirectoryStructure();

    CHECK(ds.addAttribute(DirectoryKey("subFolder"), "test"));
    CHECK(!ds.makeSubFolder(DirectoryKey("subFolder")));
}

TEST(DirectoryStructure, createFile)
{
    DirectoryStructure ds = makeDirectoryStructure();
    DirectoryKey dkey("test.file");

    CHECK(ds.createFile(dkey));
    FileDescriptor desc(100);
    CHECK(ds.updateFile(dkey, desc));

    CHECK(*ds.openFile(dkey) == desc);
    CHECK(ds.createFile(dkey));
    CHECK(*ds.openFile(dkey) == FileDescriptor());
}

TEST(DirectoryStructure, appendFile)
{
    DirectoryStructure ds = makeDirectoryStructure();
    DirectoryKey dkey("test.file");

    CHECK(*ds.appendFile(dkey) == FileDescriptor());

    FileDescriptor desc(100);
    CHECK(ds.updateFile(dkey, desc));

    CHECK(*ds.appendFile(dkey) == desc);
}

TEST(DirectoryStructure, updateFileNeedsExistingFile)
{
    DirectoryStructure ds = makeDirectoryStructure();
    DirectoryKey dkey("test.file");

    FileDescriptor desc(100);
    CHECK(!ds.updateFile(dkey, desc));
}

TEST(DirectoryStructure, nonFileEntryPreventsCreationOfFile)
{
    DirectoryStructure ds = makeDirectoryStructure();
    DirectoryKey dkey("test.file");
    ds.addAttribute(dkey, 1);

    CHECK(!ds.createFile(dkey));
    CHECK(!ds.appendFile(dkey));
    FileDescriptor desc(100);
    CHECK(!ds.updateFile(dkey, desc));
}

TEST(Cursor, creation)
{
    DirectoryStructure::Cursor cursor;
    auto cur2 = cursor;
    CHECK(cur2 == cursor);
    CHECK(!cursor);

    DirectoryStructure ds = makeDirectoryStructure();
    CHECK(ds.addAttribute(DirectoryKey("attrib"), "test"));
    auto cur3 = ds.find(DirectoryKey("attrib"));

    CHECK(cur3);
    CHECK(cur3 != cur2);
    auto res = cur3.key();
    CHECK(res.first == DirectoryKey::Root);
    CHECK(res.second == "attrib");
    CHECK(std::get<std::string>(cur3.value()) == "test");
    CHECK(cur3.getValueType() == DirectoryObjType::String);
    CHECK(cur3.getValueTypeName() == "String");

    auto cur4 = ds.next(cur3);
    CHECK(!cur4);
}
