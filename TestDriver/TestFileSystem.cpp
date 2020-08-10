
#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/SimpleFile.h"
#include "../CompoundFs/DirectoryStructure.h"
#include "../CompoundFs/Path.h"
#include "../CompoundFs/FileSystem.h"

using namespace TxFs;

namespace
{

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

TEST(FileSystem, Cursor)
{
    auto fs = makeFileSystem();
    CHECK(fs.addAttribute("folder/folder/attrib", 5));
    auto cur = fs.find("folder/folder/attrib");
    CHECK(cur);
    CHECK(std::get<int>(cur.value()) == 5);
}

TEST(FileSystem, commitClosesAllFileHandles)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    fs.close(handle);

    auto readHandle = fs.readFile("folder/file.file").value();
    auto writeHandle = fs.createFile("folder/file2.file").value();
    fs.commit();
    try
    {
        fs.close(readHandle);
        CHECK(false);
    }
    catch (std::exception&)
    {
    }

    try
    {
        fs.close(writeHandle);
        CHECK(false);
    }
    catch (std::exception&)
    {
    }
}

