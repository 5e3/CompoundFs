
#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/SimpleFile.h"
#include "../CompoundFs/CacheManager.h"
#include <algorithm>
#include <random>
#include <numeric>


using namespace TxFs;

///////////////////////////////////////////////////////////////////////////////

TEST(PrioritizedPage, sortOrder)
{
    std::vector<CacheManager::PrioritizedPage> psis;
    psis.emplace_back(PageClass::Dirty, 5, 0, 0);
    psis.emplace_back(PageClass::Dirty, 3, 5, 1);
    psis.emplace_back(PageClass::Dirty, 3, 4, 2);
    psis.emplace_back(PageClass::New, 0, 5, 3);
    psis.emplace_back(PageClass::New, 0, 4, 4);
    psis.emplace_back(PageClass::New, 0, 0, 5);
    psis.emplace_back(PageClass::Read, 3, 1, 6);
    psis.emplace_back(PageClass::Read, 3, 0, 7);
    psis.emplace_back(PageClass::Read, 2, 10, 8);

    auto psis2 = psis;
    std::shuffle(psis2.begin(), psis2.end(), std::mt19937(std::random_device()()));
    std::sort(psis2.begin(), psis2.end());
    CHECK(psis2 == psis);
}

///////////////////////////////////////////////////////////////////////////////


TEST(CacheManager, newPageIsCachedButNotWritten)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    PageIndex idx;
    {
        auto p = cm.newPage();
        idx = p.m_index;
        {
            auto p2 = cm.loadPage(p.m_index);
            CHECK(p2 == p);
            *p.m_page = 0xaa;
        }
    }
    auto p2 = cm.loadPage(idx);
    CHECK(*p2.m_page == 0xaa);
    CHECK(*sf.m_file.at(idx) != *p2.m_page);
}

TEST(CacheManager, loadPageIsCachedButNotWritten)
{
    SimpleFile sf;
    auto id = sf.newInterval(1).begin();
    *sf.m_file.at(id) = 42;

    CacheManager cm(&sf);
    auto p = cm.loadPage(id);
    auto p2 = cm.loadPage(id);
    CHECK(p == p2);

    *std::const_pointer_cast<uint8_t>(p.m_page) = 99;
    CHECK(*sf.m_file.at(id) == 42);
}

TEST(CacheManager, trimReducesSizeOfCache)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for (int i = 0; i < 10; i++)
        auto page = cm.newPage();

    CHECK(cm.trim(20) == 10);
    CHECK(cm.trim(9) == 9);
    CHECK(cm.trim(5) == 5);
    CHECK(cm.trim(0) == 0);
}

TEST(CacheManager, newPageGetsWrittenToFileOnTrim)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }

    cm.trim(0);
    for (int i = 0; i < 10; i++)
        CHECK(*sf.m_file.at(i) == i + 1);
}

TEST(CacheManager, pinnedPageDoNotGetWrittenToFileOnTrim)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }
    auto p1 = cm.loadPage(0);
    auto p2 = cm.loadPage(9);

    CHECK(cm.trim(0) == 2);

    for (int i = 1; i < 9; i++)
        CHECK(*sf.m_file.at(i) == i + 1);

    CHECK(*sf.m_file.at(0) != *p1.m_page);
    CHECK(*sf.m_file.at(9) != *p2.m_page);
}

TEST(CacheManager, newPageGetsWrittenToFileOn2TrimOps)
{
    SimpleFile sf;
    CacheManager cm(&sf);

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
        CHECK(*sf.m_file.at(i) == i + 10);
}

TEST(CacheManager, newPageDontGetWrittenToFileOn2TrimOpsWithoutSettingDirty)
{
    SimpleFile sf;
    CacheManager cm(&sf);

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
        CHECK(*sf.m_file.at(i) == i + 1);
}

TEST(CacheManager, dirtyPagesCanBeEvictedAndReadInAgain)
{
    SimpleFile sf;
    {
        CacheManager cm(&sf);
        for (int i = 0; i < 10; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i + 1;
        }
        cm.trim(0);
    }

    CacheManager cm(&sf);
    for (int i = 0; i < 10; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p = i + 10;
    }
    cm.trim(0);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.loadPage(i).m_page;
        CHECK(*p == i + 10);
    }
}

TEST(CacheManager, dirtyPagesCanBeEvictedTwiceAndReadInAgain)
{
    SimpleFile sf;
    {
        CacheManager cm(&sf);
        for (int i = 0; i < 10; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i + 1;
        }
        cm.trim(0);
    }

    CacheManager cm(&sf);
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
    CHECK(sf.m_file.size() == 20);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.loadPage(i).m_page;
        CHECK(*p == i + 20);
    }
}

TEST(CacheManager, dirtyPagesGetRedirected)
{
    SimpleFile sf;
    {
        CacheManager cm(&sf);
        for (int i = 0; i < 10; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i + 1;
        }
        cm.trim(0);
    }

    CacheManager cm(&sf);
    for (int i = 0; i < 10; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p = i + 10;
    }
    cm.trim(0);
    auto redirectedPages = cm.getRedirectedPages();
    CHECK(redirectedPages.size() == 10);
    for (auto page : redirectedPages)
        CHECK(page >= 10);
}

TEST(CacheManager, repurposedPagesCanComeFromCache)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.repurpose(i).m_page;
        CHECK(*p == i + 1);
    }
}

TEST(CacheManager, repurposedPagesAreNotLoadedIfNotInCache)
{
    SimpleFile sf;
    CacheManager cm(&sf);

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
        CHECK(*p != i + 1);
    }
}

TEST(CacheManager, setPageIndexAllocator)
{
    SimpleFile sf;
    CacheManager cm(&sf);
    cm.setPageIntervalAllocator([](size_t) { return Interval(5); });
    auto pdef = cm.newPage();
    pdef.m_page.reset();

    try
    {
        cm.trim(0);
        CHECK(false);
    }
    catch (std::exception&)
    {}
}

TEST(CacheManager, getAllDirtyPageIds)
{
    SimpleFile sf;
    {
        // create new pages
        CacheManager cm(&sf);
        for (int i = 0; i < 40; i++)
            cm.newPage();
        cm.trim(0);
    }

    // 10 dirty pages
    CacheManager cm(&sf);
    for (int i = 10; i < 20; i++)
        cm.makePageWritable(cm.loadPage(i));

    // pin one of them so it cannot be trimmed
    auto pinnedPage = cm.loadPage(15);
    cm.trim(0);

    // make one of the trimmed pages dirty again
    cm.makePageWritable(cm.loadPage(16));

    // make 10 more dirty pages that stay in the cache
    for (int i = 20; i < 30; i++)
        cm.makePageWritable(cm.loadPage(i));

    auto dirtyPageIds = cm.getAllDirtyPageIds();
    CHECK(dirtyPageIds.size() == 20);
    std::sort(dirtyPageIds.begin(), dirtyPageIds.end());

    auto expected = dirtyPageIds;
    std::iota(expected.begin(), expected.end(), 10);
    CHECK(dirtyPageIds == expected);
}

TEST(CacheManager, copyDirtyPagesMakesACopyOfTheOriginalPage)
{
    SimpleFile sf;
    {
        // perpare some pages with contents
        CacheManager cm(&sf);
        for (int i = 0; i < 50; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i;
        }
        cm.trim(0);
    }

    // make some dirty pages and trim them
    CacheManager cm(&sf);
    for (int i = 10; i < 20; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p += 100;
    }
    cm.trim(0);

    // make some more dirty and keep them in cache
    for (int i = 20; i < 30; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p += 100;
    }

    auto dirtyPageIds = cm.getAllDirtyPageIds();
    auto origToCopyPages = cm.copyDirtyPages(dirtyPageIds);

    // do the test...
    CHECK(dirtyPageIds.size() == origToCopyPages.size());
    for (auto [orig, cpy]: origToCopyPages)
    {
        CHECK(orig != cpy);
        CHECK(TxFs::isEqualPage(cm.getRawFileInterface(), orig, cpy));
        uint8_t buffer[4096];
        TxFs::readPage(cm.getRawFileInterface(), orig, buffer);
        CHECK(*buffer < 100);
    }
}

TEST(CacheManager, updateDirtyPagesChangesOriginalPages)
{
    SimpleFile sf;
    {
        // perpare some pages with contents
        CacheManager cm(&sf);
        for (int i = 0; i < 50; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i;
        }
        cm.trim(0);
    }

    // make some dirty pages and trim them
    CacheManager cm(&sf);
    for (int i = 10; i < 20; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p += 100;
    }
    cm.trim(0);

    // make some more dirty and keep them in cache
    for (int i = 20; i < 30; i++)
    {
        auto p = cm.makePageWritable(cm.loadPage(i)).m_page;
        *p += 100;
    }

    auto dirtyPageIds = cm.getAllDirtyPageIds();
    cm.updateDirtyPages(dirtyPageIds);

    // do the test...
    for (auto orig: dirtyPageIds)
    {
        uint8_t buffer[4096];
        TxFs::readPage(cm.getRawFileInterface(), orig, buffer);
        CHECK(*buffer > 100);
    }
}

TEST(CacheManager, NoLogsReturnEmpty)
{
    SimpleFile sf;
    CacheManager cm(&sf);
    CHECK(cm.readLogs().empty());

    cm.newPage();
    CHECK(cm.readLogs().empty());
}

TEST(CacheManager, ReadLogsReturnTheLogs)
{
    SimpleFile sf;
    CacheManager cm(&sf);
    cm.newPage();
    std::vector<std::pair<PageIndex, PageIndex>> logs(1000);
    std::generate(logs.begin(), logs.end(), [n = 0]() mutable { return std::make_pair(n++, n); });
    cm.writeLogs(logs);
    auto logs2 = cm.readLogs();
    std::sort(logs2.begin(), logs2.end());
    CHECK(logs == logs2);
}
