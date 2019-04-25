

#include "stdafx.h"
#include "Test.h"
#include "SimpleFile.h"
#include "MinimalTreeBuilder.h"
#include "../CompoundFs/CacheManager.h"
#include "../CompoundFs/BTree.h"
#include "../CompoundFs/ByteString.h"
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
    ByteString value("Te$tData");
    bt.insert("2233", value);
    auto res = bt.find("2233");
    CHECK(value == res.value());

    // value has different size => remove, add
    value = ByteString("Data");
    bt.insert("1122", value);
    res = bt.find("1122");
    CHECK(value == res.value());
}

TEST(BTree, insertNewKeyInsertsAndReturnsInserted)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    for (size_t i = 0; i < 3000; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), "TestData");
    }

    auto res = bt.insert("TestKey", "TestValue", [](const ByteStringView&) {
        throw std::runtime_error("");
        return true;
    });

    CHECK(std::holds_alternative<BTree::Inserted>(res));
}

TEST(BTree, canControlReplacementWithStrategy)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    for (size_t i = 0; i < 1000; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), "TestData");
    }

    bt.insert("TestKey", "TestValue", [](const ByteStringView&) {
        throw std::runtime_error("");
        return true;
    });

    auto res = bt.insert("TestKey", "TestValue1", [](const ByteStringView&) { return false; });

    CHECK(std::get<BTree::Unchanged>(res).m_currentValue.current().second == ByteString("TestValue"));

    res = bt.insert("TestKey", "TestValue2", [](const ByteStringView&) { return true; });

    CHECK(std::get<BTree::Replaced>(res).m_beforeValue == ByteString("TestValue"));
    CHECK(bt.find("TestKey").value() == ByteString("TestValue2"));
}

TEST(BTree, emptyTreeReturnsFalseCursor)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    auto cur = bt.begin("");
    CHECK(!cur);
    CHECK(!bt.next(cur));
}

TEST(BTree, cursorPointsToCurrentItem)
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
    CHECK(cur.current().first == ByteString("100"));
    CHECK(cur.current().second == ByteString("100 Test"));
}

TEST(BTree, cursorIterates)
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
        CHECK(cur);
        cur = bt.next(cur);
    }

    CHECK(!cur);
}

TEST(BTree, cursorNextPointsToNext)
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
    CHECK(cur.current().first == ByteString("101"));
}

TEST(BTree, cursorKeepsPageInMemory)
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
    CHECK(cur.current().first == ByteString("250"));
    CHECK(cur.current().second == ByteString("250 Test"));

    cur = BTree::Cursor();
    pagesStillInMem = cm->trim(0);
    CHECK(pagesStillInMem == 0);
}

TEST(BTree, removeAllKeysLeavesTreeEmpty)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    std::vector<uint32_t> keys;
    keys.reserve(MANYITERATION);
    for (uint32_t i = 0; i < MANYITERATION; i++)
        keys.push_back(i);

    for (auto key: keys)
    {
        std::string s = std::to_string(key);
        bt.insert(s.c_str(), s.c_str());
    }

    auto size = sf.m_file.size();

    for (auto key: keys)
    {
        std::string s = std::to_string(key);
        auto res = bt.remove(s.c_str());
        CHECK(res);
        CHECK(res == ByteString(s.c_str()));
    }

    CHECK(!bt.begin(""));
    CHECK(bt.getFreePages().size() == size);
}

TEST(BTree, removeNonExistantKeyReturnsEmptyOptional)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    std::vector<uint32_t> keys;
    keys.reserve(500);
    for (uint32_t i = 0; i < 500; i++)
        keys.push_back(i);

    for (auto key: keys)
    {
        std::string s = std::to_string(key);
        bt.insert(s.c_str(), (s + " Test").c_str());
    }

    CHECK(!bt.remove("Test"));
    CHECK(bt.remove("399").value() == ByteString("399 Test"));
}

TEST(BTree, removeOfSomeValuesLeavesTheOthersIntact)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    std::vector<std::string> keys;

    keys.reserve(3000);
    for (size_t i = 0; i < 3000; i++)
    {
        keys.push_back(std::to_string(i));
        bt.insert(keys.back().c_str(), keys.back().c_str());
    }

    std::shuffle(keys.begin(), keys.end(), std::mt19937(std::random_device()()));
    for (size_t i = 1000; i < 3000; i++)
    {
        auto res = bt.remove(keys[i].c_str());
        CHECK(res);
    }
    sf.clearPages(bt.getFreePages());

    for (size_t i = 0; i < 1000; i++)
    {
        auto res = bt.find(keys[i].c_str());
        CHECK(res);
    }

    for (size_t i = 1000; i < 3000; i++)
    {
        auto res = bt.find(keys[i].c_str());
        CHECK(!res);
    }

    std::sort(keys.begin(), keys.begin() + 1000);

    // make sure at least one page is completely empty
    auto size = bt.getFreePages().size();
    for (size_t i = 800; i < 1000; i++)
    {
        auto res = bt.remove(keys[i].c_str());
        CHECK(res);
    }
    CHECK(bt.getFreePages().size() > size);
    sf.clearPages(bt.getFreePages());

    auto cursor = bt.begin("");
    for (size_t i = 0; i < 800; i++)
    {
        CHECK(cursor.key() == ByteString(keys[i].c_str()));
        cursor = bt.next(cursor);
    }
    CHECK(!cursor);
}

TEST(BTree, insertAfterRemoveWorks)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    std::vector<std::string> keys;

    keys.reserve(3000);
    for (size_t i = 0; i < 3000; i++)
    {
        keys.push_back(std::to_string(i));
        bt.insert(keys.back().c_str(), keys.back().c_str());
    }

    std::shuffle(keys.begin(), keys.end(), std::mt19937(std::random_device()()));
    for (size_t i = 500; i < 3000; i++)
    {
        auto res = bt.remove(keys[i].c_str());
        CHECK(res);
    }

    for (size_t i = 500; i < 3000; i++)
    {
        auto res = bt.insert(keys[i].c_str(), keys[i].c_str());
        CHECK(!res);
    }

    for (size_t i = 0; i < 3000; i++)
    {
        auto res = bt.find(keys[i].c_str());
        CHECK(res);
    }

    std::sort(keys.begin(), keys.end());

    auto cursor = bt.begin("");
    for (size_t i = 0; i < 3000; i++)
    {
        CHECK(cursor.key() == ByteString(keys[i].c_str()));
        cursor = bt.next(cursor);
    }
    CHECK(!cursor);
}

TEST(BTree, removeInReverseOrder)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);

    std::vector<std::string> keys;

    keys.reserve(3000);
    for (size_t i = 0; i < 3000; i++)
    {
        keys.push_back(std::to_string(i));
        bt.insert(keys.back().c_str(), keys.back().c_str());
    }

    std::reverse(keys.begin(), keys.end());
    for (size_t i = 1000; i < 3000; i++)
    {
        auto res = bt.remove(keys[i].c_str());
        CHECK(res);
    }
    CHECK(bt.getFreePages().size() > 0);

    std::reverse(keys.begin(), keys.end());
    auto cursor = bt.begin("");
    for (size_t i = 2000; i < 3000; i++)
    {
        CHECK(cursor.key() == ByteString(keys[i].c_str()));
        CHECK(bt.find(ByteString(keys[i].c_str())) == cursor);
        cursor = bt.next(cursor);
    }
    CHECK(!cursor);
}

TEST(BTree, leftMergeWithDeletionOnTheLeft)
{
    SimpleFile sf;
    auto root = MinimalTreeBuilder(&sf).buildTree(4);
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm, root);

    bt.remove("0001");
    CHECK(bt.find("0003"));
}

TEST(BTree, leftMergeWithDeletionOnTheRight)
{
    SimpleFile sf;
    auto root = MinimalTreeBuilder(&sf).buildTree(4);
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm, root);

    bt.remove("0035");
    CHECK(bt.find("0033"));
}
