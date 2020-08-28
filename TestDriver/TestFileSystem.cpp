

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
    ByteStringView data("test");
    auto size = data.size();
    ASSERT_EQ(size , fs.write(handle, data.data(), data.size()));
    fs.close(handle);

    uint8_t buf[10];
    auto readHandle = fs.readFile("folder/file.file");
    ASSERT_TRUE(readHandle);
    ASSERT_EQ(size , fs.read(*readHandle, buf, sizeof(buf)));
    ASSERT_EQ(data , ByteStringView(buf,size));
}

TEST(FileSystem, readAfterAppendAfterWrite)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    ByteStringView data("test");
    auto size = data.size();
    ASSERT_EQ(size , fs.write(handle, data.data(), data.size()));
    fs.close(handle);
    handle = fs.appendFile("folder/file.file").value();
    ASSERT_EQ(size , fs.write(handle, data.data(), data.size()));
    fs.close(handle);

    uint8_t buf[20];
    auto readHandle = fs.readFile("folder/file.file");
    ASSERT_TRUE(readHandle);
    ASSERT_EQ(2 * size , fs.read(*readHandle, buf, sizeof(buf)));
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
    ASSERT_EQ(folder , folder2);
}

TEST(FileSystem, attribute)
{
    auto fs = makeFileSystem();
    auto success = fs.addAttribute("folder/attribute", "test");
    ASSERT_TRUE(success);
    success = fs.addAttribute("folder/attribute", 42.42);
    ASSERT_TRUE(success);

    auto attribute = fs.getAttribute("folder/attribute");
    ASSERT_TRUE(attribute);
    ASSERT_EQ(attribute->toValue<double>() , 42.42);
}

TEST(FileSystem, remove)
{
    auto fs = makeFileSystem();
    auto success = fs.addAttribute("folder/attribute", 42.42);
    ASSERT_TRUE(success);

    ASSERT_EQ(fs.remove("folder/attribute") , 1);
    auto attribute = fs.getAttribute("folder/attribute");
    ASSERT_TRUE(!attribute);
}

TEST(FileSystem, Cursor)
{
    auto fs = makeFileSystem();
    ASSERT_TRUE(fs.addAttribute("folder/folder/attrib", 5.5));
    auto cur = fs.find("folder/folder/attrib");
    ASSERT_TRUE(cur);
    auto attrib = cur.value();
    ASSERT_EQ(attrib.toValue<double>() , 5.5);
}

TEST(FileSystem, commitClosesAllFileHandles)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    fs.close(handle);

    auto readHandle = fs.readFile("folder/file.file").value();
    auto writeHandle = fs.createFile("folder/file2.file").value();
    fs.commit();
    ASSERT_THROW(fs.close(readHandle), std::exception);
    ASSERT_THROW(fs.close(writeHandle), std::exception);
}

TEST(FileSystem, rollbackClosesAllFileHandles)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    fs.close(handle);

    auto readHandle = fs.readFile("folder/file.file").value();
    auto writeHandle = fs.createFile("folder/file2.file").value();
    fs.rollback();
    ASSERT_THROW(fs.close(readHandle), std::exception);
    ASSERT_THROW(fs.close(writeHandle), std::exception);
}

TEST(FileSystem, fileSizeReturnsCurrentFileSize)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    ByteStringView data("test");
    ASSERT_EQ(fs.fileSize(handle), 0);
    fs.write(handle, data.data(), data.size());
    ASSERT_EQ(fs.fileSize(handle), data.size());

    fs.write(handle, data.data(), data.size());
    ASSERT_EQ(fs.fileSize(handle),2* data.size());
    fs.close(handle);

    ASSERT_EQ(*fs.fileSize("folder/file.file"), 2*data.size());
    auto handle2 = *fs.readFile("folder/file.file");
    ASSERT_EQ(fs.fileSize(handle2), 2*data.size());
}

class FileSystemTester : public ::testing::Test
{
public:
    std::shared_ptr<CacheManager> m_cacheManager;
    FileSystem m_fileSystem;
    std::string m_fileData;

    FileSystemTester()
        : m_cacheManager(std::make_shared<CacheManager>(std::make_unique<MemoryFile>()))
        , m_fileSystem(m_cacheManager, FileDescriptor(1))
        , m_fileData(5000, '1')
    {
        TypedCacheManager tcm(m_cacheManager);
        auto freeStorePage = tcm.newPage<FileTable>();
        assert(freeStorePage.m_index == 1);

        fillFileSystem();
    }

    void fillFileSystem()
    { 
        m_fileSystem.addAttribute("test/attribute", "test");
        auto handle = *m_fileSystem.createFile("test/file1.txt");
        m_fileSystem.write(handle, m_fileData.data(), m_fileData.size());
        m_fileSystem.close(handle);

        handle = *m_fileSystem.createFile("test/file2.txt");
        m_fileSystem.write(handle, m_fileData.data(), m_fileData.size());
        m_fileSystem.close(handle);
    }

    std::string readFile(ReadHandle handle)
    { 
        auto size = m_fileSystem.fileSize(handle);
        std::string data(size, ' ');
        m_fileSystem.read(handle, data.data(), size);
        return data;
    }

    void checkFileSystem()
    { 
        auto attribute = m_fileSystem.getAttribute("test/attribute").value();
        ASSERT_EQ(attribute.toValue<std::string>(), "test");

        auto handle = m_fileSystem.readFile("test/file1.txt").value();
        auto data = readFile(handle);
        ASSERT_EQ(data, m_fileData);
        data.clear();

        auto handle2 = m_fileSystem.readFile("test/file2.txt").value();
        auto data2 = readFile(handle2);
        ASSERT_EQ(data2, m_fileData);
        
        m_fileSystem.close(handle);
        m_fileSystem.close(handle2);
    }
};

TEST_F(FileSystemTester, selfTest)
{
    checkFileSystem();
}

TEST_F(FileSystemTester, afterCommitItemsStillAvailable)
{
    m_fileSystem.commit();
    checkFileSystem();
}

TEST_F(FileSystemTester, writingDataIncreasesCompositSize)
{
    auto compositSize = m_cacheManager->getFileInterface()->currentSize();
    auto handle = *m_fileSystem.createFile("test3.txt");
    m_fileSystem.write(handle, m_fileData.data(), m_fileData.size());
    m_fileSystem.close(handle);
    ASSERT_GT (m_cacheManager->getFileInterface()->currentSize(), compositSize);
}

//TEST_F(FileSystemTester, rollbackRemovesAllEntries)
//{
//    m_fileSystem.rollback();
//    ASSERT_FALSE(m_fileSystem.getAttribute("test/attribute"));
//    ASSERT_FALSE(m_fileSystem.readFile("test/file1.txt"));
//    ASSERT_FALSE(m_fileSystem.readFile("test/file2.txt"));
//}

TEST_F(FileSystemTester, rollbackReducesCompositSize)
{
    auto compositSize = m_cacheManager->getFileInterface()->currentSize();
    m_fileSystem.rollback();
    ASSERT_LT(m_cacheManager->getFileInterface()->currentSize(), compositSize);
}

TEST_F(FileSystemTester, rollbackMakesDeletedItemsReapear)
{
    m_fileSystem.commit();
    m_fileSystem.remove("test");
    ASSERT_FALSE(m_fileSystem.getAttribute("test/attribute"));
    ASSERT_FALSE(m_fileSystem.readFile("test/file1.txt"));
    ASSERT_FALSE(m_fileSystem.readFile("test/file2.txt"));

    m_fileSystem.rollback();
    checkFileSystem();
}

TEST_F(FileSystemTester, commitedDeletedSpaceGetsReused)
{
    m_fileSystem.commit();
    m_fileSystem.remove("test");
    m_fileSystem.commit();
    auto compositSize = m_cacheManager->getFileInterface()->currentSize();
    auto handle = *m_fileSystem.createFile("test3.txt");
    m_fileSystem.write(handle, m_fileData.data(), m_fileData.size());
    ASSERT_EQ(m_cacheManager->getFileInterface()->currentSize(), compositSize);
}

