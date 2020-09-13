


#include <gtest/gtest.h>
#include "CompoundFs/IntervalSequence.h"
#include <algorithm>
#include <random>

using namespace TxFs;

TEST(IntervalSequence, PushBackIsNotEmpty)
{
    IntervalSequence is;
    ASSERT_TRUE(is.empty());
    is.pushBack(Interval(0, 10));
    ASSERT_TRUE(!is.empty());
}

TEST(IntervalSequence, FillIntervals)
{
    IntervalSequence is;
    is.pushBack(Interval(0, 10));
    is.pushBack(Interval(11, 20));

    Interval iv = is.popFront();
    ASSERT_EQ(iv , Interval(0, 10));

    iv = is.popFront();
    ASSERT_EQ(iv , Interval(11, 20));
    ASSERT_TRUE(is.empty());
}

TEST(IntervalSequence, FrontDoesntRemove)
{
    IntervalSequence is;
    is.pushBack(Interval(0, 10));
    is.pushBack(Interval(11, 20));

    Interval iv = is.front();
    ASSERT_EQ(iv , Interval(0, 10));

    iv = is.front();
    ASSERT_EQ(iv , Interval(0, 10));

    is.popFront();
    iv = is.front();
    ASSERT_EQ(iv , Interval(11, 20));
}

TEST(IntervalSequence, MergeIntervals)
{
    IntervalSequence is;
    is.pushBack(Interval(0, 10));
    is.pushBack(Interval(10, 20));

    Interval iv = is.front();
    ASSERT_EQ(iv , Interval(0, 20));
}

TEST(IntervalSequence, SplitIntervals)
{
    IntervalSequence is;
    is.pushBack(Interval(0, 10));
    is.pushBack(Interval(10, 20));

    Interval iv = is.popFront(5);
    ASSERT_EQ(iv , Interval(0, 5));

    iv = is.popFront(5);
    ASSERT_EQ(iv , Interval(5, 10));

    iv = is.popFront(20);
    ASSERT_EQ(iv , Interval(10, 20));
    ASSERT_TRUE(is.empty());
}

TEST(IntervalSequence, IterateOneByOne)
{
    IntervalSequence is;
    for (int i = 0; i < 20; i += 3)
        is.pushBack(Interval(i, i + 2));

    int j = 0;
    while (!is.empty())
    {
        Interval iv = is.popFront(1);
        ASSERT_EQ(iv , Interval(j, j + 1));
        j++;
        if (((j + 1) % 3) == 0)
            j++;
    }
}

TEST(IntervalSequence, TotalLength)
{
    IntervalSequence is;
    for (int i = 0; i < 100; i += 10)
        is.pushBack(Interval(i, i + 5));

    ASSERT_EQ(is.totalLength() , 50);
}

TEST(IntervalSequence, Sort)
{
    IntervalSequence is;
    is.pushBack(Interval(10, 20));
    is.pushBack(Interval(0, 10));
    is.pushBack(Interval(20, 30));

    is.sort();
    Interval iv = is.front();
    ASSERT_EQ(iv , Interval(0, 30));

    iv = is.popFront(10);
    ASSERT_EQ(iv , Interval(0, 10));

    iv = is.popFront(10);
    ASSERT_EQ(iv , Interval(10, 20));

    iv = is.popFront(10);
    ASSERT_EQ(iv , Interval(20, 30));
}

TEST(IntervalSequence, Sort2)
{
    std::vector<PageIndex> v;
    for (int i = 0; i < 50; i++)
        v.push_back(i);

    for (int i = 51; i < 100; i += 2)
        v.push_back(i);

    std::shuffle(v.begin(), v.end(), std::mt19937(std::random_device()()));

    IntervalSequence is;
    for (const auto& id: v)
        is.pushBack(Interval(id, id + 1));

    size_t totalLen = is.totalLength();
    is.sort();
    ASSERT_EQ(totalLen , is.totalLength());

    Interval iv = is.popFront();
    ASSERT_EQ(iv , Interval(0, 50));
}
