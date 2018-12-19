
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
    for (size_t i=0; i<pages; i++)
        for(auto& writer: writers)
            writer.writeIterator(data.begin(), data.end());
            
    std::vector<FileDescriptor> fds;
    fds.reserve(files);
    for(auto& writer: writers)
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
    auto freeStorePage = pm->newFileTable();

    FileDescriptor fsfd(freeStorePage.second);
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
}

TEST(FreeStore, forEveryDeletedFileThereIsAFreePage)
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
    CHECK(fsfd.m_fileSize >= 50*4096);
}

TEST(FreeStore, smallFilesAreConsolidatedInOnePageTable)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();

    FileDescriptor fsfd(freeStorePage.second);
    FreeStore fs(pm, fsfd);

    std::vector<FileDescriptor> fileDescriptors = createFiles(pm, 100, 1);
    //for (auto& large: createFiles(pm, 3, 1100))
    //    fileDescriptors.push_back(large);

    std::random_shuffle(fileDescriptors.begin(), fileDescriptors.end());

    for (auto& fd : fileDescriptors)
        fs.deleteFile(fd);

    fsfd = fs.close();
    CHECK(fsfd.m_first == fsfd.m_last);
}

