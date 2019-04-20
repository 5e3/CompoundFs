
#include "stdafx.h"
#include "Test.h"
#include "SimpleFile.h"
#include "../CompoundFs/DirectoryStructure.h"
#include "../CompoundFs/FileSystem.h"

using namespace TxFs;

namespace
{

DirectoryStructure makeDirectoryStructure()
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();
    FileDescriptor fsfd(freeStorePage.m_index);
    return DirectoryStructure(cm, fsfd);
}

}

TEST(Path, noSubFolderDoesNotChangePath)
{
    auto ds = makeDirectoryStructure();
    Path p("test");
    Path p2 = p;

    CHECK(p.create(&ds));
    CHECK(p.m_root == Path::AbsoluteRoot);
    CHECK(p.m_relativePath == "test");
    CHECK(p == p2);
}

TEST(Path, createCreatesSubFolders)
{
    auto ds = makeDirectoryStructure();
    Path p("test/folder");
    Path p2 = p;

    CHECK(p.create(&ds));
    CHECK(p.m_root != Path::AbsoluteRoot);
    CHECK(p.m_relativePath == "folder");
    CHECK(ds.subFolder(DirectoryKey("test")));
    CHECK(p != p2);
}

TEST(Path, creationOfFolderFailsWhenTheNameIsUsedForAnAttribute)
{
    auto ds = makeDirectoryStructure();
    Path p("test/folder/attribute");

    CHECK(p.create(&ds));
    ds.addAttribute(DirectoryKey(p.m_root, p.m_relativePath), 1);
    Path p2("test/folder/attribute/test");
    Path p3 = p2;
    CHECK(!p2.create(&ds));
    CHECK(p2 == p3);

    ds.remove(DirectoryKey(p.m_root, p.m_relativePath));
    CHECK(p2.create(&ds));
    CHECK(p2 != p3);
}

TEST(Path, reduceFindsSubFolders)
{
    auto ds = makeDirectoryStructure();
    Path p("test/test/test/folder/file.file");
    Path p2 = p;
    Path p3 = p;

    CHECK(p.create(&ds));
    CHECK(p2.create(&ds));
    CHECK(p3.reduce(&ds));

    CHECK(p == p2);
    CHECK(p == p3);
    CHECK(p.m_root != Path::AbsoluteRoot);
    CHECK(p.m_relativePath == "file.file");
}
