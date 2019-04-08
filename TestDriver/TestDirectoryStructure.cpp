

#include "stdafx.h"
#include "Test.h"
#include "SimpleFile.h"
#include "MinimalTreeBuilder.h"
#include "../CompoundFs/DirectoryStructure.h"

using namespace TxFs;

DirectoryStructure makeDirectoryStructure()
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();
    FileDescriptor fsfd(freeStorePage.m_index);
    return DirectoryStructure(cm, fsfd);
}

TEST(DirectoryStructure, FoldersAreNotFound)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto res = ds.subFolder("test");
    CHECK(!res);
}

TEST(DirectoryStructure, makeFolderReturnsFolder)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto res = ds.makeSubFolder("test");
    CHECK(res);
    CHECK(*res == Folder { 1 });

    res = ds.makeSubFolder("test");
    CHECK(res);
    CHECK(*res == Folder { 1 });

    CHECK(ds.subFolder("test") == *res);
}

TEST(DirectoryStructure, makeSubFolder)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder("subFolder");
    auto subsub = ds.makeSubFolder("subsub", *subFolder);
    CHECK(subsub);
    CHECK(subsub == ds.subFolder("subsub", *subFolder));
}

TEST(DirectoryStructure, simpleRemove)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder("subFolder").value();
    auto subsub = ds.makeSubFolder("subsub", subFolder);
    auto nof = ds.remove("subsub", subFolder);
    CHECK(nof == 1);
    CHECK(!ds.subFolder("subsub", subFolder));
}

TEST(DirectoryStructure, recursiveRemove)
{
    DirectoryStructure ds = makeDirectoryStructure();

    auto subFolder = ds.makeSubFolder("subFolder").value();
    ds.makeSubFolder("subsub1", subFolder);
    ds.makeSubFolder("subsub2", subFolder);
    ds.makeSubFolder("subsub3", subFolder);
    ds.makeSubFolder("subsub4", subFolder);
    auto nof = ds.remove("subFolder");
    CHECK(nof == 5);
}
