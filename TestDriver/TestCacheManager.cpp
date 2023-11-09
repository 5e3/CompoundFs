

#include <gtest/gtest.h>
#include "CompoundFs/FileIo.h"
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/CacheManager.h"
#include "CompoundFs/CommitHandler.h"
#include "CompoundFs/RollbackHandler.h"
#include <algorithm>

using namespace TxFs;

namespace
{
    uint8_t readFirstByteFromPage(const FileInterface* fi, PageIndex idx)
    {
        uint8_t res;
        fi->readPage(idx, 0, &res, &res + 1);
        return res;
    }

    void writeFirstByteFromPage(FileInterface* fi, PageIndex idx, uint8_t val)
    {
        uint8_t page[4096];
        page[0] = val;
        writeSignedPage(fi, idx, page);
    }
    }

///////////////////////////////////////////////////////////////////////////////


TEST(CacheManager, newPageIsCachedButNotWritten)
{
    CacheManager cm(std::make_unique<MemoryFile>());

    PageIndex idx;
    {
        auto p = cm.newPage();
        idx = p.m_index;
        {
            auto p2 = cm.loadPage(p.m_index);
            ASSERT_EQ(p2 , p);
            *p.m_page = 0xaa;
        }
    }
    auto p2 = cm.loadPage(idx);
    ASSERT_EQ(*p2.m_page , 0xaa);
    ASSERT_NE(readFirstByteFromPage(cm.getFileInterface(), idx) , *p2.m_page);
}

TEST(CacheManager, loadPageIsCachedButNotWritten)
{
    auto memFile = std::make_unique<MemoryFile>();
    auto id = memFile->newInterval(1).begin();
    writeFirstByteFromPage(memFile.get(), id, 42);

    CacheManager cm(std::move(memFile));
    auto p = cm.loadPage(id);
    auto p2 = cm.loadPage(id);
    ASSERT_EQ(p , p2);

    *std::const_pointer_cast<uint8_t>(p.m_page) = 99;
    ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), id) , 42);
}

TEST(CacheManager, trimReducesSizeOfCache)
{
    CacheManager cm(std::make_unique<MemoryFile>());

    for (int i = 0; i < 10; i++)
        auto page = cm.newPage();

    ASSERT_EQ(cm.trim(20) , 10);
    ASSERT_EQ(cm.trim(9) , 9);
    ASSERT_EQ(cm.trim(5) , 5);
    ASSERT_EQ(cm.trim(0) , 0);
}

TEST(CacheManager, newPageGetsWrittenToFileOnTrim)
{
    CacheManager cm(std::make_unique<MemoryFile>());

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }

    cm.trim(0);
    for (int i = 0; i < 10; i++)
        ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), i) , i + 1);
}

TEST(CacheManager, pinnedPageDoNotGetWrittenToFileOnTrim)
{
    CacheManager cm(std::make_unique<MemoryFile>());

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 10;
    }
    auto p1 = cm.loadPage(0);
    auto p2 = cm.loadPage(9);

    ASSERT_EQ(cm.trim(0) , 2);

    for (int i = 1; i < 9; i++)
        ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), i) , i + 10);

    ASSERT_NE(readFirstByteFromPage(cm.getFileInterface(), 0) , *p1.m_page);
    ASSERT_NE(readFirstByteFromPage(cm.getFileInterface(), 9) , *p2.m_page);
}

TEST(CacheManager, newPageGetsWrittenToFileOn2TrimOps)
{
    CacheManager cm(std::make_unique<MemoryFile>());

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }

    cm.trim(0);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i));
        *p.m_page = i + 10;
    }

    cm.trim(0);

    for (int i = 0; i < 10; i++)
        ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), i) , i + 10);
}

TEST(CacheManager, newPageDontGetWrittenToFileOn2TrimOpsWithoutSettingDirty)
{
    CacheManager cm(std::make_unique<MemoryFile>());

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }

    cm.trim(0);

    for (int i = 0; i < 10; i++)
    {
        auto p = std::const_pointer_cast<uint8_t>(cm.loadPage(i).m_page); // don't do that! !!!!!!!!!!!!!!!
        *p = i + 10;                                                      // change page but no pageDirty()
    }

    cm.trim(0);

    for (int i = 0; i < 10; i++)
        ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), i) , i + 1);
}

TEST(CacheManager, dirtyPagesCanBeEvictedAndReadInAgain)
{
    std::unique_ptr<FileInterface> file = std::make_unique<MemoryFile>();
    {
        CacheManager cm(std::move(file));
        for (int i = 0; i < 10; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i + 1;
        }
        cm.trim(0);
        file = cm.handOverFile();
    }

    CacheManager cm(std::move(file));
    for (int i = 0; i < 10; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p = i + 10;
    }
    cm.trim(0);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.loadPage(i).m_page;
        ASSERT_EQ(*p , i + 10);
    }
}

TEST(CacheManager, asNewPageIsNotFollowingDirtyPageProtocol)
{
    std::unique_ptr<FileInterface> file = std::make_unique<MemoryFile>();
    {
        CacheManager cm(std::move(file));
        for (int i = 0; i < 2; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i + 1;
        }
        cm.trim(0);
        ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), 0), 1);  
        ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), 1), 2); 
        file = cm.handOverFile();
    }

    CacheManager cm(std::move(file));
    *cm.makePageWritable(cm.loadPage(0)).m_page = 10; // diverted to pageId 2
    *cm.asNewPage(1).m_page = 11;                     // written to pageId 1
    cm.trim(0);

    ASSERT_EQ(cm.getFileInterface()->fileSizeInPages(), 3);
    ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), 0), 1);  // stays untouched
    ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), 1), 11);  
    ASSERT_EQ(readFirstByteFromPage(cm.getFileInterface(), 2), 10);
}

TEST(CacheManager, dirtyPagesCanBeEvictedTwiceAndReadInAgain)
{
    std::unique_ptr<FileInterface> file = std::make_unique<MemoryFile>();
    {
        CacheManager cm(std::move(file));
        for (int i = 0; i < 10; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i + 1;
        }
        cm.trim(0);
        file = cm.handOverFile();
    }

    CacheManager cm(std::move(file));
    for (int i = 0; i < 10; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p = i + 10;
    }
    cm.trim(0);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p = i + 20;
    }
    cm.trim(0);
    ASSERT_EQ(cm.getFileInterface()->fileSizeInPages() , 20);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.loadPage(i).m_page;
        ASSERT_EQ(*p , i + 20);
    }
}

TEST(CacheManager, dirtyPagesGetDiverted)
{
    std::unique_ptr<FileInterface> file = std::make_unique<MemoryFile>();
    {
        CacheManager cm(std::move(file));
        for (int i = 0; i < 10; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i + 1;
        }
        cm.trim(0);
        file = cm.handOverFile();
    }

    CacheManager cm(std::move(file));
    for (int i = 0; i < 10; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p = i + 10;
    }
    cm.trim(0);
    auto divertedPageIds = cm.getCommitHandler().getDivertedPageIds();
    ASSERT_EQ(divertedPageIds.size() , 10);
    for (auto page : divertedPageIds)
        ASSERT_TRUE(page >= 10);
}

TEST(CacheManager, repurposedPagesCanComeFromCache)
{
    CacheManager cm(std::make_unique<MemoryFile>());

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.repurpose(i).m_page;
        ASSERT_EQ(*p , i + 1);
    }
}

TEST(CacheManager, repurposedPagesAreNotLoadedIfNotInCache)
{
    CacheManager cm(std::make_unique<MemoryFile>());

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }
    cm.trim(0);

    // make sure the PageAllocator has pages with different values
    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 100;
    }

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.repurpose(i).m_page;
        ASSERT_NE(*p , i + 1);
    }
}

TEST(CacheManager, setPageIntervalAllocator)
{
    CacheManager cm(std::make_unique<MemoryFile>());
    cm.setPageIntervalAllocator([](size_t) { return Interval(5); });
    auto pdef = cm.newPage();
    pdef.m_page.reset();
    ASSERT_THROW(cm.trim(0), std::exception);
}

TEST(CacheManager, NoLogsReturnEmpty)
{
    CacheManager cm(std::make_unique<MemoryFile>());
    auto rollbackHandler = cm.getRollbackHandler();
    ASSERT_TRUE(rollbackHandler.readLogs().empty());

    cm.newPage();
    ASSERT_TRUE(rollbackHandler.readLogs().empty());
}

TEST(CacheManager, ReadLogsReturnTheLogs)
{
    CacheManager cm(std::make_unique<MemoryFile>());
    cm.newPage();
    std::vector<std::pair<PageIndex, PageIndex>> logs(1000);
    std::generate(logs.begin(), logs.end(), [n = 0]() mutable { return std::make_pair(n++, n); });
    auto ch = cm.getCommitHandler();
    ch.writeLogs(logs);
    auto logs2 = cm.getRollbackHandler().readLogs();
    std::sort(logs2.begin(), logs2.end());
    ASSERT_EQ(logs , logs2);
}
