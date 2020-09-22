

#include <gtest/gtest.h>
#include "MinimalTreeBuilder.h"
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/CacheManager.h"
#include "CompoundFs/BTree.h"
#include "CompoundFs/ByteString.h"
#include <algorithm>
#include <random>
#include "CompoundFs/FileIo.h"

using namespace TxFs;

#ifdef _DEBUG
#define MANYITERATION 3000
#else
#define MANYITERATION 200000
#endif

TEST(BTree, trivialFind)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    BTree bt(cm);
    ASSERT_TRUE(!bt.find("test"));
}

TEST(BTree, insert)
{
    std::vector<std::string> keys;
    for (size_t i = 0; i < MANYITERATION; i++)
        keys.emplace_back(std::to_string(i));

    auto rng = std::mt19937(std::random_device()());
    std::shuffle(keys.begin(), keys.end(), rng);

    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    BTree bt(cm);
    for (const auto& key: keys)
        bt.insert(key.c_str(), "");

    std::shuffle(keys.begin(), keys.end(), rng);
    for (const auto& key: keys)
        ASSERT_TRUE(bt.find(key.c_str()));

    ASSERT_TRUE(!bt.find("gaga"));
}

TEST(BTree, insertReplacesOriginal)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
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
    ASSERT_EQ(value, res.value());

    // value has different size => remove, add
    value = ByteString("Data");
    bt.insert("1122", value);
    res = bt.find("1122");
    ASSERT_EQ(value, res.value());
}

TEST(BTree, insertNewKeyInsertsAndReturnsInserted)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    BTree bt(cm);

    for (size_t i = 0; i < 3000; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), "TestData");
    }

    auto res = bt.insert("TestKey", "TestValue", [](ByteStringView) {
        throw std::runtime_error("");
        return true;
    });

    ASSERT_TRUE(std::holds_alternative<BTree::Inserted>(res));
}

TEST(BTree, canControlReplacementWithStrategy)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    BTree bt(cm);

    for (size_t i = 0; i < 1000; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), "TestData");
    }

    bt.insert("TestKey", "TestValue", [](ByteStringView) {
        throw std::runtime_error("");
        return true;
    });

    auto res = bt.insert("TestKey", "TestValue1", [](ByteStringView) { return false; });

    ASSERT_EQ(std::get<BTree::Unchanged>(res).m_currentValue.current().second, ByteString("TestValue"));

    res = bt.insert("TestKey", "TestValue2", [](ByteStringView) { return true; });

    ASSERT_EQ(std::get<BTree::Replaced>(res).m_beforeValue, "TestValue");
    ASSERT_EQ(bt.find("TestKey").value(), "TestValue2");
}

TEST(BTree, emptyTreeReturnsFalseCursor)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    BTree bt(cm);

    auto cur = bt.begin("");
    ASSERT_TRUE(!cur);
    ASSERT_TRUE(!bt.next(cur));
}

TEST(BTree, cursorPointsToCurrentItem)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    BTree bt(cm);

    for (size_t i = 0; i < 500; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), (s + " Test").c_str());
    }

    auto cur = bt.begin("100");
    ASSERT_EQ(cur.current().first, "100");
    ASSERT_EQ(cur.current().second, "100 Test");
}

TEST(BTree, cursorIterates)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    BTree bt(cm);

    for (size_t i = 0; i < 500; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), (s + " Test").c_str());
    }

    auto cur = bt.begin("");
    for (size_t i = 0; i < 500; i++)
    {
        ASSERT_TRUE(cur);
        cur = bt.next(cur);
    }

    ASSERT_TRUE(!cur);
}

TEST(BTree, cursorNextPointsToNext)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    BTree bt(cm);

    for (size_t i = 0; i < 500; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), (s + " Test").c_str());
    }

    auto cur = bt.begin("100");
    cur = bt.next(cur);
    ASSERT_EQ(cur.current().first, "101");
}

TEST(BTree, cursorKeepsPageInMemory)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    BTree bt(cm);

    for (size_t i = 0; i < 500; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), (s + " Test").c_str());
    }

    auto cur = bt.begin("250");
    auto pagesStillInMem = cm->trim(0);

    ASSERT_EQ(pagesStillInMem, 1);
    ASSERT_EQ(cur.current().first, "250");
    ASSERT_EQ(cur.current().second, "250 Test");

    cur = BTree::Cursor();
    pagesStillInMem = cm->trim(0);
    ASSERT_EQ(pagesStillInMem , 0);
}

TEST(BTree, removeAllKeysLeavesTreeEmpty)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
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

    auto size = cm->getFileInterface()->currentSize();

    for (auto key: keys)
    {
        std::string s = std::to_string(key);
        auto res = bt.remove(s.c_str());
        ASSERT_TRUE(res);
        ASSERT_EQ(res , ByteString(s.c_str()));
    }

    ASSERT_TRUE(!bt.begin(""));
    ASSERT_EQ(bt.getFreePages().size() , size);
}

TEST(BTree, removeNonExistantKeyReturnsEmptyOptional)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
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

    ASSERT_TRUE(!bt.remove("Test"));
    ASSERT_EQ(bt.remove("399").value() , "399 Test");
}

TEST(BTree, removeOfSomeValuesLeavesTheOthersIntact)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
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
        ASSERT_TRUE(res);
    }
    TxFs::clearPages(cm->getFileInterface(), bt.getFreePages());

    for (size_t i = 0; i < 1000; i++)
    {
        auto res = bt.find(keys[i].c_str());
        ASSERT_TRUE(res);
    }

    for (size_t i = 1000; i < 3000; i++)
    {
        auto res = bt.find(keys[i].c_str());
        ASSERT_TRUE(!res);
    }

    std::sort(keys.begin(), keys.begin() + 1000);

    // make sure at least one page is completely empty
    auto size = bt.getFreePages().size();
    for (size_t i = 800; i < 1000; i++)
    {
        auto res = bt.remove(keys[i].c_str());
        ASSERT_TRUE(res);
    }
    ASSERT_TRUE(bt.getFreePages().size() > size);
    clearPages(cm->getFileInterface(), bt.getFreePages());

    auto cursor = bt.begin("");
    for (size_t i = 0; i < 800; i++)
    {
        ASSERT_EQ(cursor.key() , keys[i]);
        cursor = bt.next(cursor);
    }
    ASSERT_TRUE(!cursor);
}

TEST(BTree, insertAfterRemoveWorks)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
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
        ASSERT_TRUE(res);
    }

    for (size_t i = 500; i < 3000; i++)
    {
        auto res = bt.insert(keys[i].c_str(), keys[i].c_str());
        ASSERT_TRUE(!res);
    }

    for (size_t i = 0; i < 3000; i++)
    {
        auto res = bt.find(keys[i].c_str());
        ASSERT_TRUE(res);
    }

    std::sort(keys.begin(), keys.end());

    auto cursor = bt.begin("");
    for (size_t i = 0; i < 3000; i++)
    {
        ASSERT_EQ(cursor.key() , keys[i]);
        cursor = bt.next(cursor);
    }
    ASSERT_TRUE(!cursor);
}

TEST(BTree, removeInReverseOrder)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
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
        ASSERT_TRUE(res);
    }
    ASSERT_TRUE(!bt.getFreePages().empty());

    std::reverse(keys.begin(), keys.end());
    auto cursor = bt.begin("");
    for (size_t i = 2000; i < 3000; i++)
    {
        ASSERT_EQ(cursor.key() , keys[i]);
        ASSERT_EQ(bt.find(keys[i]) , cursor);
        cursor = bt.next(cursor);
    }
    ASSERT_TRUE(!cursor);
}

TEST(BTree, leftMergeWithDeletionOnTheLeft)
{
    MinimalTreeBuilder mtb( std::make_unique<MemoryFile>() );
    auto root = mtb.buildTree(4);
    auto cm = std::make_shared<CacheManager>(mtb.m_cacheManager->handOverFile());
    BTree bt(cm, root);

    bt.remove("0001");
    ASSERT_TRUE(bt.find("0003"));
}

TEST(BTree, leftMergeWithDeletionOnTheRight)
{
    MinimalTreeBuilder mtb(std::make_unique<MemoryFile>());
    auto root = mtb.buildTree(4);
    auto cm = std::make_shared<CacheManager>(mtb.m_cacheManager->handOverFile());
    BTree bt(cm, root);

     bt.remove("0035");
    ASSERT_TRUE(bt.find("0033"));
}
