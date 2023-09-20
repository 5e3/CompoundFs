

#include <gtest/gtest.h>
#include "FileSystemUtility.h"
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/DirectoryStructure.h"
#include "CompoundFs/Path.h"
#include "CompoundFs/FileSystemHelper.h"

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

TEST(FileSystemHelper, retrieveFolderContents)
{
    auto fs = makeFileSystem();
    createFile("folder/file1", fs);
    createFile("folder/file2", fs);
    createFile("folder/subFolder/file3", fs);

    auto contents = retrieveFolderContents("folder", fs);
    ASSERT_EQ(contents.size(), 3);
}

TEST(FileSystemHelper, FolderFileToFolderFile2)
{
    auto fs = makeFileSystem();
    createFile("folder/file1", fs);

    ASSERT_EQ(copy(fs, "folder/file1", "folder/file2"), 1);
    ASSERT_EQ(fs.fileSize("folder/file2"), 4);
}

TEST(FileSystemHelper, RootFileToFolderFile)
{
    auto fs = makeFileSystem();
    createFile("file1", fs);

    ASSERT_EQ(copy(fs, "file1", "folder/file2"), 1);
    ASSERT_EQ(fs.fileSize("folder/file2"), 4);
}

TEST(FileSystemHelper, folderToFolder2)
{
    auto fs = makeFileSystem();
    createFile("folder/file1", fs);
    createFile("folder/file2", fs);
    createFile("folder/subFolder/file3", fs);

    ASSERT_EQ(copy(fs, "folder", "folder2"), 5);
    ASSERT_EQ(copy(fs, "folder", "folder2"), 5);
    ASSERT_EQ(copy(fs, "folder", ""), 5);
}
