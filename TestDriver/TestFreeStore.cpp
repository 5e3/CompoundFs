
#include "stdafx.h"
#include "Test.h"
#include "SimpleFile.h"
#include "../CompoundFs/FreeStore.h"
#include "../CompoundFs/FileWriter.h"
#include "../CompoundFs/ByteString.h"
#include "../CompoundFs/TypedCacheManager.h"

#include <random>

using namespace TxFs;

FileDescriptor createFile(std::shared_ptr<CacheManager> cm)
{
    FileWriter rfw(cm);
    ByteString data("X");
    rfw.write(data.begin(), data.end());
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

/////////////////////////////////////////////////////////////////////////////////

TEST(FreeStore, closeReturnsCorrectFileDescriptor)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);

    FileDescriptor fsfd(1000); // not supposed to access it => crash
    FreeStore fs(cm, fsfd);

    CHECK(fs.close() == fsfd);
}

TEST(FreeStore, deleteOneFileIsIncludedInFileDescriptor)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    FreeStore fs(cm, fsfd);

    FileDescriptor fd = createFile(cm);

    fs.deleteFile(fd);
    CHECK(fsfd != fs.close());
    CHECK(fsfd.m_first == freeStorePage.m_index);
}

TEST(FreeStore, forEveryDeletedFileThereIsAtLeastAFreePage)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
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
    CHECK(is.totalLength() >= 50);
    CHECK(fsfd.m_fileSize >= 50 * 4096);
    CHECK(fsfd.m_first == freeStorePage.m_index);
}

TEST(FreeStore, smallFilesAreConsolidatedInOnePageTable)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf, 1600);
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    FreeStore fs(cm, fsfd);

    std::vector<FileDescriptor> fileDescriptors = createFiles(cm, 1500, 5);

    std::shuffle(fileDescriptors.begin(), fileDescriptors.end(), std::mt19937(std::random_device()()));

    for (auto& fd: fileDescriptors)
        fs.deleteFile(fd);

    fsfd = fs.close();
    CHECK(fsfd.m_first == fsfd.m_last);
    CHECK(fsfd.m_first == freeStorePage.m_index);

    auto is = readAllFreeStorePages(cm, fsfd.m_first);
    CHECK(is.size() == 1);
    CHECK(fsfd.m_fileSize == is.totalLength() * 4096ULL);
}

TEST(FreeStore, deleteBigAndSmallFiles)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf, 1600);
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
    CHECK(fsfd.m_fileSize == is.totalLength() * 4096ULL);
}

TEST(FreeStore, emptyFreeStoreReturnsEmptyInterval)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);

    FileDescriptor fsfd(1000); // crashes if accessed
    FreeStore fs(cm, fsfd);

    CHECK(fs.allocate(1) == Interval());

    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
}

TEST(FreeStore, deletedFileCanBeAllocatedAfterClose)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    {
        FreeStore fs(cm, fsfd);
        fs.deleteFile(createFile(cm));
        CHECK(fs.allocate(1) == Interval()); // available after close
        fsfd = fs.close();
    }
    {
        FreeStore fs(cm, fsfd);
        CHECK(fs.allocate(2).length() == 2);
        CHECK(fs.allocate(1).length() == 0);

        fsfd = fs.close();
        CHECK(fsfd.m_fileSize == 0);
    }

    FreeStore fs(cm, fsfd);
    CHECK(fs.allocate(1).length() == 0);

    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
}

TEST(FreeStore, deallocatedPagesAreAvailableAfterClose)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    TypedCacheManager tcm(cm);
    auto freeStorePage = tcm.newPage<FileTable>();

    FileDescriptor fsfd(freeStorePage.m_index);
    {
        FreeStore fs(cm, fsfd);
        for (int i = 0; i < 10; i++)
            fs.deallocate(tcm.newPage<FileTable>().m_index);

        CHECK(fs.allocate(1) == Interval()); // available after close
        fsfd = fs.close();
    }

    {
        FreeStore fs(cm, fsfd);
        CHECK(fs.allocate(5).length() == 5);
        CHECK(fs.allocate(5).length() == 5);
        CHECK(fs.allocate(5).length() == 0);

        fsfd = fs.close();
        CHECK(fsfd.m_fileSize == 0);
    }

    FreeStore fs(cm, fsfd);
    CHECK(fs.allocate(5).length() == 0);

    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
}

TEST(FreeStore, singlePageConsumed)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
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
        CHECK(fs.allocate(1).length() == 1);
        fsfd = fs.close();
        CHECK(fsfd.m_fileSize == 0);
    }

    FreeStore fs(cm, fsfd);
    CHECK(fs.allocate(1).length() == 0);
    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
}

TEST(FreeStore, deleteBigAndSmallFilesAndAllocateUntilEmpty)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf, 1600);
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
        CHECK(fsfd.m_fileSize != 0);
    }
    {
        FreeStore fs(cm, fsfd);
        while (!fs.allocate(2000).empty())
            ;
        fsfd = fs.close();
        CHECK(fsfd.m_fileSize == 0);
        CHECK(fsfd.m_first == freeStorePage.m_index);
        CHECK(fsfd.m_last == fsfd.m_first);
    }

    FreeStore fs(cm, fsfd);
    CHECK(!fs.allocate(1).empty() == 0);
    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
    CHECK(fsfd.m_first == freeStorePage.m_index);
    CHECK(fsfd.m_last == fsfd.m_first);
}
