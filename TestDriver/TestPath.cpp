

#include "Test.h"
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/Path.h"
#include "../CompoundFs/DirectoryStructure.h"
#include "../CompoundFs/FileSystem.h"

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
    Path p("folder/folder2");
    Path p2 = p;

    CHECK(p.create(&ds));
    CHECK(p.m_root != Path::AbsoluteRoot);
    CHECK(p.m_relativePath == "folder2");
    CHECK(ds.subFolder(DirectoryKey("folder")));
    CHECK(p != p2);
}

TEST(Path, creationOfFolderFailsWhenTheNameIsUsedForAnAttribute)
{
    auto ds = makeDirectoryStructure();
    Path p("folder/folder/attribute");

    CHECK(p.create(&ds));
    ds.addAttribute(DirectoryKey(p.m_root, p.m_relativePath), 1);
    Path p2("folder/folder/attribute/test");
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
    Path p("folder/folder/folder/folder/file.file");
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

