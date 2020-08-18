


#include <gtest/gtest.h>
#include "../CompoundFs/FileTable.h"
#include <list>
#include <algorithm>
#include <random>

using namespace TxFs;

TEST(FileTable, Empty)
{
    FileTable ft;
    ASSERT_TRUE(sizeof(ft) == 4096);

    IntervalSequence is;
    ft.insertInto(is);
    ASSERT_TRUE(is.empty());
}

TEST(FileTable, transferBackAndForth)
{
    IntervalSequence is;
    is.pushBack(Interval(0, 10));
    is.pushBack(Interval(11, 20));
    is.pushBack(Interval(21, 30));

    IntervalSequence is2 = is;
    FileTable ft;
    ft.transferFrom(is);
    ASSERT_TRUE(is.empty());

    ft.insertInto(is);
    ASSERT_TRUE(is == is2);
}

TEST(FileTable, transferNotEnoughSpace)
{
    IntervalSequence is;
    for (uint32_t i = 0; i < 1500; i++)
        is.pushBack(Interval(i * 2, i * 2 + 1));

    IntervalSequence is2 = is;
    FileTable ft;
    ft.transferFrom(is);
    ASSERT_TRUE(!is.empty());

    IntervalSequence is3;
    ft.insertInto(is3);
    while (!is.empty())
        is3.pushBack(is.popFront());

    ASSERT_TRUE(is3 == is2);
}

TEST(FileTable, transferNotEnoughSpace2)
{
    IntervalSequence is;
    for (uint32_t i = 0; i < 500; i++)
        is.pushBack(Interval(i * 3, i * 3 + 2));

    IntervalSequence is2 = is;
    FileTable ft;
    ft.transferFrom(is);
    ASSERT_TRUE(!is.empty());

    IntervalSequence is3;
    ft.insertInto(is3);
    while (!is.empty())
        is3.pushBack(is.popFront());

    ASSERT_TRUE(is3 == is2);
}

TEST(FileTable, transferNotEnoughSpace3)
{
    std::vector<Interval> ivect;
    for (uint32_t i = 0; i < 2000; i++)
        ivect.push_back(Interval(i * 3, i * 3 + i % 2 + 1));

    std::shuffle(ivect.begin(), ivect.end(), std::mt19937(std::random_device()()));
    IntervalSequence is;
    for (auto& iv: is)
        is.pushBack(iv);

    IntervalSequence is2 = is;
    std::list<FileTable> fts;
    while (!is.empty())
    {
        FileTable ft;
        ft.transferFrom(is);
        fts.push_back(ft);
    }

    IntervalSequence is3;
    for (auto& ft: fts)
        ft.insertInto(is3);

    ASSERT_TRUE(is3 == is2);
}
