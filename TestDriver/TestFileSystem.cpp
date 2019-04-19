
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

TEST(Path, createCreatesSubFolders)
{
    auto ds = makeDirectoryStructure();
    Path p("test/folder");
    Path p2 = p;

    p.create(&ds);
    CHECK(p.m_root != Path::AbsoluteRoot);
    CHECK(p.m_relativePath == "folder");
    CHECK(ds.subFolder(DirectoryKey("test")));
    CHECK(p != p2);
}

TEST(Path, emptyFolderThrows)
{
    auto ds = makeDirectoryStructure();
    Path p("test//folder");

    try
    {
        p.create(&ds);
        CHECK(false);
    }
    catch (std::exception&)
    {}
}
