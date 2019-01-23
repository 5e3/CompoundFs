

#include "stdafx.h"
#include <set>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include "Test.h"
#include "../CompoundFs/PageAllocator.h"

using namespace TxFs;

TEST(PageAllocator, allocReturnsWritableMemory)
{
    PageAllocator alloc(16);
    auto ptr = alloc.allocate();
    *ptr = 1;
}

TEST(PageAllocator, CanAllocateManyPages)
{
    PageAllocator alloc(16);
    std::vector<std::shared_ptr<uint8_t>> pages;
    for (int i = 0; i < 64; i++)
        pages.push_back(alloc.allocate());

    std::unordered_set<std::shared_ptr<uint8_t>> pset;
    for (auto p: pages)
        CHECK(pset.insert(p).second); // check if unique
}

TEST(PageAllocator, allocReusesMemory)
{
    PageAllocator alloc(16);

    std::unordered_set<uint8_t*> pset;
    {
        std::vector<std::shared_ptr<uint8_t>> pages;
        for (int i = 0; i < 70; i++)
        {
            pages.push_back(alloc.allocate());
            pset.insert(pages.back().get());
        }
    }

    std::vector<std::shared_ptr<uint8_t>> pages;
    for (int i = 0; i < 70; i++)
    {
        pages.push_back(alloc.allocate());
        CHECK(pset.count(pages.back().get()) != 0);
    }
}

TEST(PageAllocator, trimFreesAllButOnePageWithoutOutstandingReferences)
{
    PageAllocator alloc(16);

    {
        std::vector<std::shared_ptr<uint8_t>> pages;
        for (int i = 0; i < 200; i++)
            pages.push_back(alloc.allocate());
        std::random_shuffle(pages.begin(), pages.end());
    }

    auto statistic = alloc.trim();
    CHECK(statistic.first == 1);
    CHECK(statistic.second < 16);
}

TEST(PageAllocator, trimFreesWhatIsUsefull)
{
    PageAllocator alloc(16);
    auto p = alloc.allocate(); // hold the first page

    {
        std::vector<std::shared_ptr<uint8_t>> pages;
        for (int i = 0; i < 200; i++)
            pages.push_back(alloc.allocate());
        std::random_shuffle(pages.begin(), pages.end());
    }

    auto statistic = alloc.trim();
    CHECK(statistic.first == 2);
    CHECK(statistic.second > 16);
    CHECK(statistic.second < 32);

    p.reset();
    statistic = alloc.trim();
    CHECK(statistic.first == 1);
    CHECK(statistic.second < 16);
}

TEST(PageAllocator, stressTest)
{
    PageAllocator alloc(512);

    {
        std::vector<std::shared_ptr<uint8_t>> pages;
        for (int i = 0; i < 5120; i++)
            pages.push_back(alloc.allocate());
        std::random_shuffle(pages.begin(), pages.end());
    }

    auto statistic = alloc.trim();
    CHECK(statistic.first == 1);
    CHECK(statistic.second < 16);
}
