

#include <gtest/gtest.h>
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/DirectoryStructure.h"
#include "../CompoundFs/Path.h"
#include "../CompoundFs/FileSystem.h"

using namespace TxFs;

namespace
{

FileSystem makeFileSystem()
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();
    FileDescriptor fsfd(freeStorePage.m_index);
    return FileSystem(cm, fsfd);
}

}

TEST(FileSystem, createFile)
{
    auto fs = makeFileSystem();
    ASSERT_TRUE(fs.createFile("folder/file.file"));
}

TEST(FileSystem, appendFile)
{
    auto fs = makeFileSystem();
    auto handle = fs.appendFile("folder/file.file");
    ASSERT_TRUE(handle);
}

TEST(FileSystem, appendFileWithClose)
{
    auto fs = makeFileSystem();
    auto handle = fs.appendFile("folder/file.file");

    fs.close(*handle);
    handle = fs.appendFile("folder/file.file");
    ASSERT_TRUE(handle);
}

TEST(FileSystem, readAfterWrite)
{
    auto fs = makeFileSystem();
    auto handle = fs.appendFile("folder/file.file").value();
    ByteString data("test");
    auto size = data.size();
    ASSERT_TRUE(size == fs.write(handle, data.begin(), data.size()));
    fs.close(handle);

    uint8_t buf[10];
    auto readHandle = fs.readFile("folder/file.file");
    ASSERT_TRUE(readHandle);
    ASSERT_TRUE(size == fs.read(*readHandle, buf, sizeof(buf)));
    ASSERT_TRUE(data == ByteString(buf));
}

TEST(FileSystem, readAfterAppendAfterWrite)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    ByteString data("test");
    auto size = data.size();
    ASSERT_TRUE(size == fs.write(handle, data.begin(), data.size()));
    fs.close(handle);
    handle = fs.appendFile("folder/file.file").value();
    ASSERT_TRUE(size == fs.write(handle, data.begin(), data.size()));
    fs.close(handle);

    uint8_t buf[20];
    auto readHandle = fs.readFile("folder/file.file");
    ASSERT_TRUE(readHandle);
    ASSERT_TRUE(2 * size == fs.read(*readHandle, buf, sizeof(buf)));
}

TEST(FileSystem, doubleCloseWriteHandleThrows)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    fs.close(handle);
    try
    {
        fs.close(handle);
        ASSERT_TRUE(false);
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
        ASSERT_TRUE(false);
    }
    catch (std::exception&)
    {}
}

TEST(FileSystem, subFolder)
{
    auto fs = makeFileSystem();
    auto folder = fs.makeSubFolder("test/folder");
    ASSERT_TRUE(folder);

    auto folder2 = fs.subFolder("test/folder");
    ASSERT_TRUE(folder2);
    ASSERT_TRUE(folder == folder2);
}

TEST(FileSystem, attribute)
{
    auto fs = makeFileSystem();
    auto success = fs.addAttribute("folder/attribute", 42);
    ASSERT_TRUE(success);
    success = fs.addAttribute("folder/attribute", 42.42);
    ASSERT_TRUE(success);

    auto attribute = fs.getAttribute("folder/attribute");
    ASSERT_TRUE(attribute);
    ASSERT_TRUE(*attribute == ByteStringOps::Variant(42.42));
}

TEST(FileSystem, remove)
{
    auto fs = makeFileSystem();
    auto success = fs.addAttribute("folder/attribute", 42);
    ASSERT_TRUE(success);

    ASSERT_TRUE(fs.remove("folder/attribute") == 1);
    auto attribute = fs.getAttribute("folder/attribute");
    ASSERT_TRUE(!attribute);
}

TEST(FileSystem, Cursor)
{
    auto fs = makeFileSystem();
    ASSERT_TRUE(fs.addAttribute("folder/folder/attrib", 5));
    auto cur = fs.find("folder/folder/attrib");
    ASSERT_TRUE(cur);
    ASSERT_TRUE(std::get<int>(cur.value()) == 5);
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
        ASSERT_TRUE(false);
    }
    catch (std::exception&)
    {
    }

    try
    {
        fs.close(writeHandle);
        ASSERT_TRUE(false);
    }
    catch (std::exception&)
    {
    }
}

