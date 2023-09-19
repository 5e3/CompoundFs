

#include <gtest/gtest.h>
#include "FileSystemUtility.h"
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/DirectoryStructure.h"
#include "CompoundFs/Path.h"
#include "CompoundFs/FileSystem.h"

using namespace TxFs;

namespace
{

FileSystem makeFileSystem()
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    auto fs = FileSystem(FileSystem::initialize(cm));
    fs.commit();
    return fs;
}

void createFile(Path path, FileSystem& fs)
{
    auto fh = *fs.createFile(path);
    ByteStringView data("test");
    fs.write(fh, data.data(), data.size());
    fs.close(fh);
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
    ASSERT_EQ(size, fs.write(handle, data.data(), data.size()));
    fs.close(handle);

    uint8_t buf[10];
    auto readHandle = fs.readFile("folder/file.file");
    ASSERT_TRUE(readHandle);
    ASSERT_EQ(size, fs.read(*readHandle, buf, sizeof(buf)));
    ASSERT_EQ(data, ByteStringView(buf, static_cast<uint8_t>(size)));
}

TEST(FileSystem, readAfterAppendAfterWrite)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    ByteStringView data("test");
    auto size = data.size();
    ASSERT_EQ(size, fs.write(handle, data.data(), data.size()));
    fs.close(handle);
    handle = fs.appendFile("folder/file.file").value();
    ASSERT_EQ(size, fs.write(handle, data.data(), data.size()));
    fs.close(handle);

    uint8_t buf[20];
    auto readHandle = fs.readFile("folder/file.file");
    ASSERT_TRUE(readHandle);
    ASSERT_EQ(2 * size, fs.read(*readHandle, buf, sizeof(buf)));
}

TEST(FileSystem, doubleCloseWriteHandleThrows)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    fs.close(handle);
    ASSERT_THROW(fs.close(handle), std::exception);
}

TEST(FileSystem, doubleCloseReadHandleThrows)
{
    auto fs = makeFileSystem();
    auto handle = fs.createFile("folder/file.file").value();
    fs.close(handle);

    auto readHandle = fs.readFile("folder/file.file").value();
    fs.close(readHandle);
    ASSERT_THROW(fs.close(handle), std::exception);
}

TEST(FileSystem, subFolder)
{
    auto fs = makeFileSystem();
    auto folder = fs.makeSubFolder("test/folder");
    ASSERT_TRUE(folder);

    auto folder2 = fs.subFolder("test/folder");
    ASSERT_TRUE(folder2);
    ASSERT_EQ(folder, folder2);
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
    ASSERT_EQ(attribute->toValue<double>(), 42.42);
}

TEST(FileSystem, remove)
{
    auto fs = makeFileSystem();
    auto success = fs.addAttribute("folder/attribute", 42.42);
    ASSERT_TRUE(success);

    ASSERT_EQ(fs.remove("folder/attribute"), 1);
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
    ASSERT_EQ(attrib.toValue<double>(), 5.5);
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
    ASSERT_EQ(fs.fileSize(handle), 2 * data.size());
    fs.close(handle);

    ASSERT_EQ(*fs.fileSize("folder/file.file"), 2 * data.size());
    auto handle2 = *fs.readFile("folder/file.file");
    ASSERT_EQ(fs.fileSize(handle2), 2 * data.size());
}

FileSystem prepareFileSystemWithFiles()
{
    auto fs = makeFileSystem();
    createFile("folder/file.file", fs);
    createFile("folder/file2.file", fs);
    createFile("folder/subFolder/file3.file", fs);
    createFile("folder/subFolder/file4.file", fs);
    return fs;
}

TEST(FileSystem, renameMovesDirectory)
{
    auto fs = prepareFileSystemWithFiles();

    ASSERT_TRUE(fs.rename("folder/subFolder", "folder2"));
    ASSERT_FALSE(fs.subFolder("folder/subFolder"));
    ASSERT_TRUE(fs.readFile("folder2/file3.file"));
    ASSERT_TRUE(fs.readFile("folder2/file4.file"));
}

TEST(FileSystem, renameWhereSourceDoesntExistsFails)
{
    auto fs = prepareFileSystemWithFiles();

    ASSERT_FALSE(fs.rename("folder/file3", "file5"));
}

TEST(FileSystem, renameWhereDestinationIsNotAFolderFails)
{
    auto fs = prepareFileSystemWithFiles();

    ASSERT_FALSE(fs.rename("folder/subFolder/file3.file", "folder/file.file/subFolder/file3.file"));
}

TEST(FileSystem, renameCanMoveAFile)
{
    auto fs = prepareFileSystemWithFiles();

    ASSERT_TRUE(fs.rename("folder/subFolder/file3.file", "folder2/subFolder/file3.file"));
    ASSERT_TRUE(fs.readFile("folder2/subFolder/file3.file"));
    ASSERT_FALSE(fs.readFile("folder/subFolder/file3.file"));
}

TEST(FileSystem, CannotRenameRootFolder)
{
    auto fs = prepareFileSystemWithFiles();

    ASSERT_FALSE(fs.rename("", "newRoot"));
}

class FileSystemTester : public ::testing::Test
{
public:
    std::shared_ptr<CacheManager> m_cacheManager;
    FileSystem m_fileSystem;
    FileSystemUtility m_helper;

    FileSystemTester()
        : m_cacheManager(std::make_shared<CacheManager>(std::make_unique<MemoryFile>()))
        , m_fileSystem(FileSystem::initialize(m_cacheManager))
    {
        m_fileSystem.commit();
        m_helper.fillFileSystem(m_fileSystem);
    }
};

TEST_F(FileSystemTester, selfTest)
{
    m_helper.checkFileSystem(m_fileSystem);
}

TEST_F(FileSystemTester, afterCommitItemsStillAvailable)
{
    m_fileSystem.commit();
    m_helper.checkFileSystem(m_fileSystem);
}

TEST_F(FileSystemTester, writingDataIncreasesCompositSize)
{
    auto compositSize = m_cacheManager->getFileInterface()->fileSizeInPages();
    auto handle = *m_fileSystem.createFile("test3.txt");
    m_fileSystem.write(handle, m_helper.m_fileData.data(), m_helper.m_fileData.size());
    m_fileSystem.close(handle);
    ASSERT_GT(m_cacheManager->getFileInterface()->fileSizeInPages(), compositSize);
}

TEST_F(FileSystemTester, rollbackRemovesAllEntries)
{
    m_fileSystem.rollback();
    ASSERT_FALSE(m_fileSystem.getAttribute("test/attribute"));
    ASSERT_FALSE(m_fileSystem.readFile("test/file1.txt"));
    ASSERT_FALSE(m_fileSystem.readFile("test/file2.txt"));
}

TEST_F(FileSystemTester, rollbackReducesCompositSize)
{
    auto compositSize = m_cacheManager->getFileInterface()->fileSizeInPages();
    m_fileSystem.rollback();
    ASSERT_LT(m_cacheManager->getFileInterface()->fileSizeInPages(), compositSize);
}

TEST_F(FileSystemTester, rollbackMakesDeletedItemsReapear)
{
    m_fileSystem.commit();
    m_fileSystem.remove("test");
    ASSERT_FALSE(m_fileSystem.getAttribute("test/attribute"));
    ASSERT_FALSE(m_fileSystem.readFile("test/file1.txt"));
    ASSERT_FALSE(m_fileSystem.readFile("test/file2.txt"));

    m_fileSystem.rollback();
    m_helper.checkFileSystem(m_fileSystem);
}

TEST_F(FileSystemTester, commitedDeletedSpaceGetsReused)
{
    m_fileSystem.commit();
    m_fileSystem.remove("test");
    m_fileSystem.commit();
    auto compositSize = m_cacheManager->getFileInterface()->fileSizeInPages();
    auto handle = *m_fileSystem.createFile("test3.txt");
    m_fileSystem.write(handle, m_helper.m_fileData.data(), m_helper.m_fileData.size());
    ASSERT_EQ(m_cacheManager->getFileInterface()->fileSizeInPages(), compositSize);
}

TEST_F(FileSystemTester, treeSpaceGetsReused)
{
    m_fileSystem.rollback();
    auto file = m_cacheManager->getFileInterface();
    uint64_t i = 0;
    auto csize = file->fileSizeInPages();
    while (csize == file->fileSizeInPages())
    {
        auto data = std::to_string(i++);
        m_fileSystem.addAttribute(Path(data), data);
    }
    m_fileSystem.commit();
    csize = file->fileSizeInPages();
    for (uint64_t j = 0; j < i; j++)
        ASSERT_EQ(m_fileSystem.remove(Path(std::to_string(j))), 1);
    m_fileSystem.commit();
    for (uint64_t j = 0; j < i; j++)
    {
        auto data = std::to_string(j);
        m_fileSystem.addAttribute(Path(data), data);
    }
    ASSERT_EQ(file->fileSizeInPages(), csize);
}
