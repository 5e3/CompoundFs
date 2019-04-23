
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
    auto sf = new SimpleFile;
    auto cm = std::make_shared<CacheManager>(sf);
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();
    FileDescriptor fsfd(freeStorePage.m_index);
    return DirectoryStructure(cm, fsfd);
}

FileSystem makeFileSystem()
{
    auto sf = new SimpleFile;
    auto cm = std::make_shared<CacheManager>(sf);
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();
    FileDescriptor fsfd(freeStorePage.m_index);
    return FileSystem(cm, fsfd);
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

TEST(FileSystem, createFile)
{
    auto fs = makeFileSystem();
    CHECK(fs.createFile("folder/file.file"));
}

TEST(FileSystem, appendFile)
{
    auto fs = makeFileSystem();
    auto handle = fs.appendFile("folder/file.file");
    CHECK(handle);
}

TEST(FileSystem, appendFileWithClose)
{
    auto fs = makeFileSystem();
    auto handle = fs.appendFile("folder/file.file");

    fs.close(*handle);
    handle = fs.appendFile("folder/file.file");
    CHECK(handle);
}

TEST(FileSystem, readAfterWrite)
{
    auto fs = makeFileSystem();
    auto handle = fs.appendFile("folder/file.file").value();
    ByteString data("test");
    auto size = data.size();
    CHECK(size == fs.write(handle, data.begin(), data.size()));
    fs.close(handle);

    uint8_t buf[10];
    auto readHandle = fs.readFile("folder/file.file");
    CHECK(readHandle);
    CHECK(size == fs.read(*readHandle, buf, sizeof(buf)));
    CHECK(data == ByteString(buf));
}

TEST(FileSystem, readAfterAppendAfterWrite)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    ByteString data("test");
    auto size = data.size();
    CHECK(size == fs.write(handle, data.begin(), data.size()));
    fs.close(handle);
    handle = fs.appendFile("folder/file.file").value();
    CHECK(size == fs.write(handle, data.begin(), data.size()));
    fs.close(handle);

    uint8_t buf[20];
    auto readHandle = fs.readFile("folder/file.file");
    CHECK(readHandle);
    CHECK(2 * size == fs.read(*readHandle, buf, sizeof(buf)));
}

TEST(FileSystem, doubleCloseWriteHandleThrows)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    fs.close(handle);
    try
    {
        fs.close(handle);
        CHECK(false);
    }
    catch (std::exception&)
    {}
}

TEST(FileSystem, doubleCloseReadHandleThrows)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    fs.close(handle);

    auto readHandle = fs.readFile("folder/file.file").value();
    fs.close(readHandle);
    try
    {
        fs.close(readHandle);
        CHECK(false);
    }
    catch (std::exception&)
    {}
}

TEST(FileSystem, subFolder)
{
    auto fs = makeFileSystem();
    auto folder = fs.makeSubFolder("test/folder");
    CHECK(folder);

    auto folder2 = fs.subFolder("test/folder");
    CHECK(folder2);
    CHECK(folder == folder2);
}

TEST(FileSystem, attribute)
{
    auto fs = makeFileSystem();
    auto success = fs.addAttribute("folder/attribute", 42);
    CHECK(success);
    success = fs.addAttribute("folder/attribute", 42.42);
    CHECK(success);

    auto attribute = fs.getAttribute("folder/attribute");
    CHECK(attribute);
    CHECK(*attribute == ByteStringOps::Variant(42.42));
}

TEST(FileSystem, remove)
{
    auto fs = makeFileSystem();
    auto success = fs.addAttribute("folder/attribute", 42);
    CHECK(success);

    CHECK(fs.remove("folder/attribute") == 1);
    auto attribute = fs.getAttribute("folder/attribute");
    CHECK(!attribute);
}
