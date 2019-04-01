

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
    CHECK(value == res.value());

    // value has different size => remove, add
    value = Blob("Data");
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

    auto res = bt.insert("TestKey", "TestValue", [](const BlobRef&, const BlobRef&) {
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

    bt.insert("TestKey", "TestValue", [](const BlobRef&, const BlobRef&) {
        throw std::runtime_error("");
        return true;
    });

    auto res = bt.insert("TestKey", "TestValue1", [](const BlobRef&, const BlobRef&) { return false; });

    CHECK(std::get<BTree::Unchanged>(res).m_currentValue.current().second == Blob("TestValue"));

    res = bt.insert("TestKey", "TestValue2", [](const BlobRef&, const BlobRef&) { return true; });

    CHECK(std::get<BTree::Replaced>(res).m_beforeValue == Blob("TestValue"));
    CHECK(bt.find("TestKey").value() == Blob("TestValue2"));
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
    CHECK(cur.current().first == Blob("100"));
    CHECK(cur.current().second == Blob("100 Test"));
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
    CHECK(cur.current().first == Blob("101"));
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
    CHECK(cur.current().first == Blob("250"));
    CHECK(cur.current().second == Blob("250 Test"));

    cur = Cursor();
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
        bt.insert(s.c_str(), "TestData");
    }

    for (auto key: keys)
    {
        std::string s = std::to_string(key);
        auto res = bt.remove(s.c_str());
        CHECK(res);
        CHECK(res == Blob("TestData"));
    }

    CHECK(!bt.begin(""));
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
    CHECK(bt.remove("399").value() == Blob("399 Test"));
}
