

#include <gtest/gtest.h>
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/FreeStore.h"
#include "CompoundFs/FileWriter.h"
#include "CompoundFs/ByteString.h"
#include "CompoundFs/TypedCacheManager.h"

#include <random>

using namespace TxFs;

namespace 
{

FileDescriptor createFile(std::shared_ptr<CacheManager> cm)
{
    FileWriter rfw(cm);
    ByteStringView data("X");
    rfw.write(data.data(), data.end());
    return rfw.close();
}

std::vector<FileDescriptor> createFiles(std::shared_ptr<CacheManager> cm, size_t files, size_t pages)
{
    std::vector<uint8_t> data(4096, 'Y');
    std::vector<FileWriter> writers(files, cm);
    for (size_t i = 0; i < pages; i++)
        for (auto& writer: writers)
            writer.writeIterator(data.begin(), data.end());

    std::vector<FileDescriptor> fds;
    fds.reserve(files);
    for (auto& writer: writers)
        fds.push_back(writer.close());

    return fds;
}

IntervalSequence readAllFreeStorePages(std::shared_ptr<CacheManager> cm, PageIndex firstFreeStorePage)
{
    PageIndex page = firstFreeStorePage;
    IntervalSequence is;
    TypedCacheManager tcm(cm);
    while (page != PageIdx::INVALID)
    {
        auto fileTablePage = tcm.loadPage<FileTable>(page).m_page;
        fileTablePage->insertInto(is);
        page = fileTablePage->getNext();
    }
    return is;
}
}
/////////////////////////////////////////////////////////////////////////////////

TEST(FreeStore, closeReturnsCorrectFileDescriptor)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());

    FileDescriptor fsfd(1000); // not supposed to access it => crash
    FreeStore fs(cm, fsfd);

    ASSERT_EQ(fs.close() , fsfd);
}

TEST(FreeStore, deleteOneFileIsIncludedInFileDescriptor)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    FreeStore fs(cm, fsfd);

    FileDescriptor fd = createFile(cm);

    fs.deleteFile(fd);
    ASSERT_NE(fsfd , fs.close());
    ASSERT_EQ(fsfd.m_first , freeStorePage.m_index);
}

TEST(FreeStore, forEveryDeletedFileThereIsAtLeastAFreePage)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    FreeStore fs(cm, fsfd);

    std::vector<FileDescriptor> fileDescriptors;
    for (int i = 0; i < 50; i++)
        fileDescriptors.push_back(createFile(cm));

    for (auto& fd: fileDescriptors)
        fs.deleteFile(fd);

    fsfd = fs.close();
    auto is = readAllFreeStorePages(cm, fsfd.m_first);
    ASSERT_TRUE(is.totalLength() >= 50);
    ASSERT_TRUE(fsfd.m_fileSize >= 50 * 4096);
    ASSERT_EQ(fsfd.m_first , freeStorePage.m_index);
}

TEST(FreeStore, smallFilesAreConsolidatedInOnePageTable)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    FreeStore fs(cm, fsfd);

    std::vector<FileDescriptor> fileDescriptors = createFiles(cm, 1500, 5);

    std::shuffle(fileDescriptors.begin(), fileDescriptors.end(), std::mt19937(std::random_device()()));

    for (auto& fd: fileDescriptors)
        fs.deleteFile(fd);

    fsfd = fs.close();
    ASSERT_EQ(fsfd.m_first , fsfd.m_last);
    ASSERT_EQ(fsfd.m_first , freeStorePage.m_index);

    auto is = readAllFreeStorePages(cm, fsfd.m_first);
    ASSERT_EQ(is.size() , 1);
    ASSERT_EQ(fsfd.m_fileSize , is.totalLength() * 4096ULL);
}

TEST(FreeStore, deleteBigAndSmallFiles)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    FreeStore fs(cm, fsfd);

    std::vector<FileDescriptor> fileDescriptors = createFiles(cm, 500, 5);
    for (auto& large: createFiles(cm, 3, 2200))
        fileDescriptors.push_back(large);

    std::shuffle(fileDescriptors.begin(), fileDescriptors.end(), std::mt19937(std::random_device()()));

    for (auto& fd: fileDescriptors)
        fs.deleteFile(fd);

    fsfd = fs.close();
    auto is = readAllFreeStorePages(cm, fsfd.m_first);
    ASSERT_EQ(fsfd.m_fileSize , is.totalLength() * 4096ULL);
}

TEST(FreeStore, emptyFreeStoreReturnsEmptyInterval)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());

    FileDescriptor fsfd(1000); // crashes if accessed
    FreeStore fs(cm, fsfd);

    ASSERT_EQ(fs.allocate(1) , Interval());

    fsfd = fs.close();
    ASSERT_EQ(fsfd.m_fileSize , 0);
}

TEST(FreeStore, deletedFileCanBeAllocatedAfterClose)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    {
        FreeStore fs(cm, fsfd);
        fs.deleteFile(createFile(cm));
        ASSERT_EQ(fs.allocate(1) , Interval()); // available after close
        fsfd = fs.close();
    }
    {
        FreeStore fs(cm, fsfd);
        ASSERT_EQ(fs.allocate(2).length() , 2);
        ASSERT_EQ(fs.allocate(1).length() , 0);

        fsfd = fs.close();
        ASSERT_EQ(fsfd.m_fileSize , 0);
    }

    FreeStore fs(cm, fsfd);
    ASSERT_EQ(fs.allocate(1).length() , 0);

    fsfd = fs.close();
    ASSERT_EQ(fsfd.m_fileSize , 0);
}

TEST(FreeStore, deallocatedPagesAreAvailableAfterClose)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    {
        FreeStore fs(cm, fsfd);
        for (int i = 0; i < 10; i++)
            fs.deallocate(tcm.newPage<FileTable>().m_index);

        ASSERT_EQ(fs.allocate(1) , Interval()); // available after close
        fsfd = fs.close();
    }

    {
        FreeStore fs(cm, fsfd);
        ASSERT_EQ(fs.allocate(5).length() , 5);
        ASSERT_EQ(fs.allocate(5).length() , 5);
        ASSERT_EQ(fs.allocate(5).length() , 0);

        fsfd = fs.close();
        ASSERT_EQ(fsfd.m_fileSize , 0);
    }

    FreeStore fs(cm, fsfd);
    ASSERT_EQ(fs.allocate(5).length() , 0);

    fsfd = fs.close();
    ASSERT_EQ(fsfd.m_fileSize , 0);
}

TEST(FreeStore, singlePageConsumed)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    {
        FreeStore fs(cm, fsfd);
        fs.deallocate(tcm.newPage<FileTable>().m_index);
        fsfd = fs.close();
    }

    {
        FreeStore fs(cm, fsfd);
        ASSERT_EQ(fs.allocate(1).length() , 1);
        fsfd = fs.close();
        ASSERT_EQ(fsfd.m_fileSize , 0);
    }

    FreeStore fs(cm, fsfd);
    ASSERT_EQ(fs.allocate(1).length() , 0);
    fsfd = fs.close();
    ASSERT_EQ(fsfd.m_fileSize , 0);
}

TEST(FreeStore, deleteBigAndSmallFilesAndAllocateUntilEmpty)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();
    FileDescriptor fsfd(freeStorePage.m_index);
    {
        FreeStore fs(cm, fsfd);

        std::vector<FileDescriptor> fileDescriptors = createFiles(cm, 1500, 1);
        for (auto& large: createFiles(cm, 3, 2200))
            fileDescriptors.push_back(large);

        std::shuffle(fileDescriptors.begin(), fileDescriptors.end(), std::mt19937(std::random_device()()));

        for (auto& fd: fileDescriptors)
            fs.deleteFile(fd);

        fsfd = fs.close();
    }
    {
        FreeStore fs(cm, fsfd);
        while (!fs.allocate(251).empty())
            ;
        fsfd = fs.close();
        ASSERT_NE(fsfd.m_fileSize , 0);
    }
    {
        FreeStore fs(cm, fsfd);
        while (!fs.allocate(2000).empty())
            ;
        fsfd = fs.close();
        ASSERT_EQ(fsfd.m_fileSize , 0);
        ASSERT_EQ(fsfd.m_first , freeStorePage.m_index);
        ASSERT_EQ(fsfd.m_last , fsfd.m_first);
    }

    FreeStore fs(cm, fsfd);
    ASSERT_EQ(!fs.allocate(1).empty() , 0);
    fsfd = fs.close();
    ASSERT_EQ(fsfd.m_fileSize , 0);
    ASSERT_EQ(fsfd.m_first , freeStorePage.m_index);
    ASSERT_EQ(fsfd.m_last , fsfd.m_first);
}
