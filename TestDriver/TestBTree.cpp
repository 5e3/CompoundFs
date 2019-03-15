

#include "stdafx.h"
#include "Test.h"
#include "SimpleFile.h"
#include "../CompoundFs/CacheManager.h"
#include "../CompoundFs/BTree.h"
#include "../CompoundFs/Blob.h"
#include <algorithm>
#include <random>

using namespace TxFs;

#ifdef _DEBUG
#define MANYITERATION 3000
#else
#define MANYITERATION 200000
#endif

TEST(BTree, trivialFind)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);
    CHECK(!bt.find("test"));
}

TEST(BTree, insert)
{
    std::vector<std::string> keys;
    for (size_t i = 0; i < MANYITERATION; i++)
        keys.emplace_back(std::to_string(i));

    auto rng = std::mt19937(std::random_device()());
    std::shuffle(keys.begin(), keys.end(), rng);

    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);
    for (const auto& key: keys)
        bt.insert(key.c_str(), "");

    std::shuffle(keys.begin(), keys.end(), rng);
    for (const auto& key: keys)
        CHECK(bt.find(key.c_str()));

    CHECK(!bt.find("gaga"));
}

TEST(BTree, insertReplacesOriginal)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    for (size_t i = 0; i < 3000; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), "TestData");
    }

    // value has same size => inplace
    Blob value("Te$tData");
    bt.insert("2233", value);
    auto res = bt.find("2233");
    CHECK(value == res);

    // value has different size => remove, add
    value = Blob("Data");
    bt.insert("1122", value);
    res = bt.find("1122");
    CHECK(value == res);
}

TEST(BTree, EmptyTreeReturnsDoneCursor)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    auto cur = bt.begin("");
    CHECK(cur.done());
    CHECK(bt.next(cur).done());
}

TEST(BTree, CursorPointsToCurrentItem)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    for (size_t i = 0; i < 500; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), (s + " Test").c_str());
    }

    auto cur = bt.begin("100");
    CHECK(cur.current().first == Blob("100"));
    CHECK(cur.current().second == Blob("100 Test"));

    
}

TEST(BTree, CursorIterates)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    for (size_t i = 0; i < 500; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), (s + " Test").c_str());
    }

    auto cur = bt.begin("");
    for (size_t i = 0; i < 500; i++)
    {
        CHECK(!cur.done());
        cur = bt.next(cur);
    }

    CHECK(cur.done());
}

TEST(BTree, CursorNextPointsToNext)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    for (size_t i = 0; i < 500; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), (s + " Test").c_str());
    }

    auto cur = bt.begin("100");
    cur = bt.next(cur);
    CHECK(cur.current().first == Blob("101"));
}

TEST(BTree, CursorKeepsPageInMemory)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    for (size_t i = 0; i < 500; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), (s + " Test").c_str());
    }

    auto cur = bt.begin("250");
    auto pagesStillInMem = cm->trim(0);

    CHECK(pagesStillInMem == 1);
    CHECK(cur.current().first == Blob("250"));
    CHECK(cur.current().second == Blob("250 Test"));

    cur = Cursor();
    pagesStillInMem = cm->trim(0);
    CHECK(pagesStillInMem == 0);
}

