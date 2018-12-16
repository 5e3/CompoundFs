

#include "stdafx.h"
#include "Test.h"
#include "IntervalSequence.h"

using namespace CompFs;

TEST(IntervalSequence, PushBackIsNotEmpty)
{
    IntervalSequence is;
    CHECK(is.empty());
    is.pushBack(std::make_pair(0, 10));
    CHECK(!is.empty());
}

TEST(IntervalSequence, FillIntervals)
{
    IntervalSequence is;
    is.pushBack(std::make_pair(0, 10));
    is.pushBack(std::make_pair(11, 20));

    IntervalSequence::Interval iv = is.popFront();
    CHECK(iv == IntervalSequence::Interval(0, 10));

    iv = is.popFront();
    CHECK(iv == IntervalSequence::Interval(11, 20));
    CHECK(is.empty());
}

TEST(IntervalSequence, FrontDoesntRemove)
{
    IntervalSequence is;
    is.pushBack(std::make_pair(0, 10));
    is.pushBack(std::make_pair(11, 20));

    IntervalSequence::Interval iv = is.front();
    CHECK(iv == IntervalSequence::Interval(0, 10));

    iv = is.front();
    CHECK(iv == IntervalSequence::Interval(0, 10));

    is.popFront();
    iv = is.front();
    CHECK(iv == IntervalSequence::Interval(11, 20));
}

TEST(IntervalSequence, MergeIntervals)
{
    IntervalSequence is;
    is.pushBack(std::make_pair(0, 10));
    is.pushBack(std::make_pair(10, 20));

    IntervalSequence::Interval iv = is.front();
    CHECK(iv == IntervalSequence::Interval(0, 20));
}

TEST(IntervalSequence, SplitIntervals)
{
    IntervalSequence is;
    is.pushBack(std::make_pair(0, 10));
    is.pushBack(std::make_pair(10, 20));

    IntervalSequence::Interval iv = is.popFront(5);
    CHECK(iv == IntervalSequence::Interval(0, 5));

    iv = is.popFront(5);
    CHECK(iv == IntervalSequence::Interval(5, 10));

    iv = is.popFront(20);
    CHECK(iv == IntervalSequence::Interval(10, 20));
    CHECK(is.empty());
}

TEST(IntervalSequence, IterateOneByOne)
{
    IntervalSequence is;
    for (int i=0; i<20; i+=3)
        is.pushBack(std::make_pair(i, i+2));

    int j = 0;
    while (!is.empty())
    {
        IntervalSequence::Interval iv = is.popFront(1);
        CHECK(iv == IntervalSequence::Interval(j, j+1));
        j++;
        if (((j+1) % 3) == 0)
            j++;
    }
}

TEST(IntervalSequence, TotalLength)
{
    IntervalSequence is;
    for (int i = 0; i < 100; i += 10)
        is.pushBack(std::make_pair(i, i + 5));

    CHECK(is.totalLength() == 50);
}

TEST(IntervalSequence, Sort)
{
    IntervalSequence is;
    is.pushBack(std::make_pair(10, 20));
    is.pushBack(std::make_pair(0, 10));
    is.pushBack(std::make_pair(20, 30));

    is.sort();
    IntervalSequence::Interval iv = is.front();
    CHECK(iv == IntervalSequence::Interval(0, 30));

    iv = is.popFront(10);
    CHECK(iv == IntervalSequence::Interval(0, 10));

    iv = is.popFront(10);
    CHECK(iv == IntervalSequence::Interval(10, 20));

    iv = is.popFront(10);
    CHECK(iv == IntervalSequence::Interval(20, 30));

}

TEST(IntervalSequence, Sort2)
{
    std::vector<Node::Id> v;
    for (int i = 0; i < 50; i++)
        v.push_back(i);

    for (int i = 51; i < 100; i += 2)
        v.push_back(i);

    std::random_shuffle(v.begin(), v.end());

    IntervalSequence is;
    for (auto it = v.begin(); it < v.end(); ++it)
        is.pushBack(IntervalSequence::Interval(*it, (*it) + 1));

    size_t totalLen = is.totalLength();
    is.sort();
    CHECK(totalLen == is.totalLength());

    IntervalSequence::Interval iv = is.popFront();
    CHECK(iv == IntervalSequence::Interval(0, 50));
}

