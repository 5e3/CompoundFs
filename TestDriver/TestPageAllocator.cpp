


#include <set>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <random>
#include <gtest/gtest.h>
#include "CompoundFs/PageAllocator.h"

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
        ASSERT_TRUE(pset.insert(p).second); // check if unique
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
        ASSERT_NE(pset.count(pages.back().get()) , 0);
    }
}

TEST(PageAllocator, trimFreesAllButOnePageWithoutOutstandingReferences)
{
    PageAllocator alloc(16);

    {
        std::vector<std::shared_ptr<uint8_t>> pages;
        for (int i = 0; i < 200; i++)
            pages.push_back(alloc.allocate());
        std::shuffle(pages.begin(), pages.end(), std::mt19937(std::random_device()()));
    }

    auto statistic = alloc.trim();
    ASSERT_EQ(statistic.first , 1);
    ASSERT_TRUE(statistic.second < 16);
}

TEST(PageAllocator, trimFreesWhatIsUsefull)
{
    PageAllocator alloc(16);
    auto p = alloc.allocate(); // hold the first page

    {
        std::vector<std::shared_ptr<uint8_t>> pages;
        for (int i = 0; i < 200; i++)
            pages.push_back(alloc.allocate());
        std::shuffle(pages.begin(), pages.end(), std::mt19937(std::random_device()()));
    }

    auto statistic = alloc.trim();
    ASSERT_EQ(statistic.first , 2);
    ASSERT_TRUE(statistic.second > 16);
    ASSERT_TRUE(statistic.second < 32);

    p.reset();
    statistic = alloc.trim();
    ASSERT_EQ(statistic.first , 1);
    ASSERT_TRUE(statistic.second < 16);
}

TEST(PageAllocator, allocAfterTrim)
{
    PageAllocator alloc(16);

    {
        std::vector<std::shared_ptr<uint8_t>> pages(16);
        std::generate(pages.begin(), pages.end(), [pa = &alloc]() { return pa->allocate(); });
    }

    auto stat = alloc.trim();
    auto page = alloc.allocate();
    ASSERT_EQ(stat.first , 1 && stat.second == 0);
}

TEST(PageAllocator, defaultCtorTrimIsNoOp)
{
    PageAllocator alloc;
    auto stat = alloc.trim();
    ASSERT_EQ(stat.first , 0 && stat.second == 0);
}

TEST(PageAllocator, MovedAllocatorTrimIsNoOp)
{
    PageAllocator alloc;
    {
        auto p = alloc.allocate();
        PageAllocator alloc2 = std::move(alloc);
        p = alloc2.allocate();
        p = alloc.allocate();
        auto stat = alloc2.trim();
        ASSERT_TRUE(stat.first = 1 && stat.second == 2);
    }

    auto stat = alloc.trim();
    ASSERT_EQ(stat.first , 1 && stat.second == 1);
}

TEST(PageAllocator, stressTest)
{
    PageAllocator alloc(512);

    {
        std::vector<std::shared_ptr<uint8_t>> pages;
        for (int i = 0; i < 5120; i++)
            pages.push_back(alloc.allocate());
        std::shuffle(pages.begin(), pages.end(), std::mt19937(std::random_device()()));
    }

    auto statistic = alloc.trim();
    ASSERT_EQ(statistic.first , 1);
    ASSERT_TRUE(statistic.second < 16);
}

TEST(PageAllocator, moveTransfersAllInternalsToNewObject)
{
    PageAllocator alloc(16);
    std::vector<std::shared_ptr<uint8_t>> pages(20);
    std::generate(pages.begin(), pages.end(), [alloc = &alloc]() { return alloc->allocate(); });

    PageAllocator alloc2(std::move(alloc));
    pages.clear();

    auto statistic = alloc2.trim();
    ASSERT_EQ(statistic.first , 1 && statistic.second == 4);
}