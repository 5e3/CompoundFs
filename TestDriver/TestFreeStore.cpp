
#include "stdafx.h"
#include "Test.h"
#include "FreeStore.h"
#include "File.h"
#include "Blob.h"

using namespace CompFs;


TEST(FreeStore, closeReturnsCorrectFileDescriptor)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    auto freeStorePage = pm->newFileTable();

    FileDescriptor fsfd(freeStorePage.second);
    FreeStore fs(pm, fsfd);

    CHECK(fs.close() == fsfd);
}

FileDescriptor createFile(std::shared_ptr<PageManager> pm)
{
    RawFileWriter rfw(pm);
    Blob data("Test");
    rfw.write(data.begin(), data.end());
    return rfw.close();
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

TEST(FreeStore, deleteOneFile)
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

    auto is = readAllFreeStorePages(pm, fs.close().m_first);
    CHECK(is.totalLength() == 50);
}

