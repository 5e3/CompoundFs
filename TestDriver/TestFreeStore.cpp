
#include "stdafx.h"
#include "Test.h"
#include "FreeStore.h"
#include "File.h"
#include "Blob.h"

using namespace CompFs;

FileDescriptor createFile(std::shared_ptr<PageManager> pm)
{
    RawFileWriter rfw(pm);
    Blob data("X");
    rfw.write(data.begin(), data.end());
    return rfw.close();
}

std::vector<FileDescriptor> createFiles(std::shared_ptr<PageManager> pm, size_t files, size_t pages)
{
    std::vector<uint8_t> data(4096, 'Y');
    std::vector<RawFileWriter> writers(files, pm);
    for (size_t i = 0; i < pages; i++)
        for (auto& writer : writers)
            writer.writeIterator(data.begin(), data.end());

    std::vector<FileDescriptor> fds;
    fds.reserve(files);
    for (auto& writer : writers)
        fds.push_back(writer.close());

    return fds;
}

IntervalSequence readAllFreeStorePages(std::shared_ptr<PageManager> pm, Node::Id firstFreeStorePage)
{
    Node::Id page = firstFreeStorePage;
    IntervalSequence is;
    while (page != Node::INVALID_NODE)
    {
        auto fileTablePage = pm->loadFileTable(page).first;
        fileTablePage->insertInto(is);
        page = fileTablePage->getNext();
    }
    return is;
}

///////////////////////////////////////////////////////////////////////////////

TEST(FreeStore, closeReturnsCorrectFileDescriptor)
{
    std::shared_ptr<PageManager> pm(new PageManager);

    FileDescriptor fsfd(1000); // not supposed to access it => crash
    FreeStore fs(pm, fsfd);

    CHECK(fs.close() == fsfd);
}

TEST(FreeStore, deleteOneFileIsIncludedInFileDescriptor)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();

    FileDescriptor fsfd(freeStorePage.second);
    FreeStore fs(pm, fsfd);

    FileDescriptor fd = createFile(pm);

    fs.deleteFile(fd);
    CHECK(fsfd != fs.close());
    CHECK(fsfd.m_first == freeStorePage.second);
}

TEST(FreeStore, forEveryDeletedFileThereIsAtLeastAFreePage)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();

    FileDescriptor fsfd(freeStorePage.second);
    FreeStore fs(pm, fsfd);

    std::vector<FileDescriptor> fileDescriptors;
    for (int i = 0; i < 50; i++)
        fileDescriptors.push_back(createFile(pm));

    for (auto& fd : fileDescriptors)
        fs.deleteFile(fd);

    fsfd = fs.close();
    auto is = readAllFreeStorePages(pm, fsfd.m_first);
    CHECK(is.totalLength() >= 50);
    CHECK(fsfd.m_fileSize >= 50 * 4096);
    CHECK(fsfd.m_first == freeStorePage.second);
}

TEST(FreeStore, smallFilesAreConsolidatedInOnePageTable)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();

    FileDescriptor fsfd(freeStorePage.second);
    FreeStore fs(pm, fsfd);

    std::vector<FileDescriptor> fileDescriptors = createFiles(pm, 1500, 5);

    std::random_shuffle(fileDescriptors.begin(), fileDescriptors.end());

    for (auto& fd : fileDescriptors)
        fs.deleteFile(fd);

    fsfd = fs.close();
    CHECK(fsfd.m_first == fsfd.m_last);
    CHECK(fsfd.m_first == freeStorePage.second);

    auto is = readAllFreeStorePages(pm, fsfd.m_first);
    CHECK(is.size() == 1);
    CHECK(fsfd.m_fileSize == is.totalLength() * 4096);
}

TEST(FreeStore, deleteBigAndSmallFiles)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();

    FileDescriptor fsfd(freeStorePage.second);
    FreeStore fs(pm, fsfd);

    std::vector<FileDescriptor> fileDescriptors = createFiles(pm, 1500, 5);
    for (auto& large: createFiles(pm, 3, 2200))
        fileDescriptors.push_back(large);

    std::random_shuffle(fileDescriptors.begin(), fileDescriptors.end());

    for (auto& fd : fileDescriptors)
        fs.deleteFile(fd);

    fsfd = fs.close();
    auto is = readAllFreeStorePages(pm, fsfd.m_first);
    CHECK(is.size() == 3*2200 + 1);
    CHECK(fsfd.m_fileSize == is.totalLength() * 4096);
}

TEST(FreeStore, emptyFreeStoreReturnsEmptyInterval)
{
    std::shared_ptr<PageManager> pm(new PageManager);

    FileDescriptor fsfd(1000); // crashes if accessed
    FreeStore fs(pm, fsfd);

    CHECK(fs.allocate(1) == Interval());
    
    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
}

TEST(FreeStore, deletedFileCanBeAllocatedAfterClose)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();
    
    FileDescriptor fsfd(freeStorePage.second);
    {
        FreeStore fs(pm, fsfd);
        fs.deleteFile(createFile(pm));
        CHECK(fs.allocate(1) == Interval()); // available after close
        fsfd = fs.close();
    }
    {
        FreeStore fs(pm, fsfd);
        CHECK(fs.allocate(2).length() == 2); 
        CHECK(fs.allocate(1).length() == 0);
    
        fsfd = fs.close();
        CHECK(fsfd.m_fileSize == 0);
    }

    FreeStore fs(pm, fsfd);
    CHECK(fs.allocate(1).length() == 0);
    
    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
}

TEST(FreeStore, deallocatedPagesAreAvailableAfterClose)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();
    
    FileDescriptor fsfd(freeStorePage.second);
    {
        FreeStore fs(pm, fsfd);
        for (int i=0; i<10; i++)
            fs.deallocate(pm->newFileTable().second);

        CHECK(fs.allocate(1) == Interval()); // available after close
        fsfd = fs.close();
    }

    {
        FreeStore fs(pm, fsfd);
        CHECK(fs.allocate(5).length() == 5); 
        CHECK(fs.allocate(5).length() == 5); 
        CHECK(fs.allocate(5).length() == 0); 
    
        fsfd = fs.close();
        CHECK(fsfd.m_fileSize == 0);
    }

    FreeStore fs(pm, fsfd);
    CHECK(fs.allocate(5).length() == 0); 
    
    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
}

TEST(FreeStore, singlePageConsumed)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();
    
    FileDescriptor fsfd(freeStorePage.second);
    {
        FreeStore fs(pm, fsfd);
        fs.deallocate(pm->newFileTable().second);
        fsfd = fs.close();
    }

    {
        FreeStore fs(pm, fsfd);
        CHECK(fs.allocate(1).length() == 1);     
        fsfd = fs.close();
        CHECK(fsfd.m_fileSize == 0);
    }

    FreeStore fs(pm, fsfd);
    CHECK(fs.allocate(1).length() == 0); 
    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
}

TEST(FreeStore, deleteBigAndSmallFilesAndAllocateUntilEmpty)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();
    FileDescriptor fsfd(freeStorePage.second);
    {
        FreeStore fs(pm, fsfd);

        std::vector<FileDescriptor> fileDescriptors = createFiles(pm, 1500, 1);
        for (auto& large: createFiles(pm, 3, 2200))
            fileDescriptors.push_back(large);

        std::random_shuffle(fileDescriptors.begin(), fileDescriptors.end());

        for (auto& fd : fileDescriptors)
            fs.deleteFile(fd);

        fsfd = fs.close();
    }
    {
        FreeStore fs(pm, fsfd);
        while (fs.allocate(5).length());
        fsfd = fs.close();
    }
    {
        FreeStore fs(pm, fsfd);
        while (fs.allocate(2000).length());
        fsfd = fs.close();
        CHECK(fsfd.m_fileSize == 0);
        CHECK(fsfd.m_first == freeStorePage.second);
        CHECK(fsfd.m_last == fsfd.m_first);
    }
    FreeStore fs(pm, fsfd);
    CHECK(fs.allocate(1).length() == 0); 
    fsfd = fs.close();
    CHECK(fsfd.m_fileSize == 0);
    CHECK(fsfd.m_first == freeStorePage.second);
    CHECK(fsfd.m_last == fsfd.m_first);


}