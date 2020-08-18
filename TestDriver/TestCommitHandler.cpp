

#include "Test.h"
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/CacheManager.h"
#include "../CompoundFs/CommitHandler.h"
#include <random>
#include <numeric>

using namespace TxFs;


TEST(CommitHandler, getDirtyPageIds)
{
    std::unique_ptr<RawFileInterface> file = std::make_unique<MemoryFile>();
    {
        // create new pages
        CacheManager cm(std::move(file));
        for (int i = 0; i < 40; i++)
            cm.newPage();
        cm.trim(0);
        file = cm.handOverFile();
    }

    // 10 dirty pages
    CacheManager cm(std::move(file));
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

    auto dirtyPageIds = cm.buildCommitHandler().getDirtyPageIds();
    CHECK(dirtyPageIds.size() == 20);
    std::sort(dirtyPageIds.begin(), dirtyPageIds.end());

    auto expected = dirtyPageIds;
    std::iota(expected.begin(), expected.end(), 10);
    CHECK(dirtyPageIds == expected);
}

TEST(CommitHandler, copyDirtyPagesMakesACopyOfTheOriginalPage)
{
    std::unique_ptr<RawFileInterface> file = std::make_unique<MemoryFile>();
    {
        // prepare some pages with contents
        CacheManager cm(std::move(file));
        for (int i = 0; i < 50; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i;
        }
        cm.trim(0);
        file = cm.handOverFile();
    }

    // make some dirty pages and trim them
    CacheManager cm(std::move(file));
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

    auto commitHandler = cm.buildCommitHandler();
    auto dirtyPageIds = commitHandler.getDirtyPageIds();
    auto origToCopyPages = commitHandler.copyDirtyPages(dirtyPageIds);

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

TEST(CommitHandler, updateDirtyPagesChangesOriginalPages)
{
    std::unique_ptr<RawFileInterface> file = std::make_unique<MemoryFile>();
    {
        // prepare some pages with contents
        CacheManager cm(std::move(file));
        for (int i = 0; i < 50; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i;
        }
        cm.trim(0);
        file = cm.handOverFile();
    }

    // make some dirty pages and trim them
    CacheManager cm(std::move(file));
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

    auto commitHandler = cm.buildCommitHandler();
    auto dirtyPageIds = commitHandler.getDirtyPageIds();
    commitHandler.updateDirtyPages(dirtyPageIds);

    // do the test...
    for (auto orig: dirtyPageIds)
    {
        uint8_t buffer[4096];
        TxFs::readPage(cm.getRawFileInterface(), orig, buffer);
        CHECK(*buffer > 100);
    }
}
