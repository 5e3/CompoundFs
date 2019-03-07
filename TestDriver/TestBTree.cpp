

#include "stdafx.h"
#include "Test.h"
#include "SimpleFile.h"
#include "../CompoundFs/CacheManager.h"
#include "../CompoundFs/BTree.h"
#include "../CompoundFs/Blob.h"
#include <algorithm>

using namespace TxFs;

#ifdef _DEBUG
#define MANYITERATION 3000
#else
#define MANYITERATION 200000
#endif

TEST(BTree, insert)
{
    std::vector<std::string> keys;
    for (size_t i = 0; i < MANYITERATION; i++)
        keys.emplace_back(std::to_string(i));
    std::random_shuffle(keys.begin(), keys.end());

    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
    BTree bt(cm);
    for (auto& key: keys)
        bt.insert(key.c_str(), "");

    std::random_shuffle(keys.begin(), keys.end());
    Blob value;
    for (auto& key: keys)
        CHECK(bt.find(key.c_str(), value));

    CHECK(!bt.find("gaga", value));
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
    Blob res;
    bt.insert("2233", value);
    CHECK(bt.find("2233", res));
    CHECK(value == res);

    // value has different size => remove, add
    value = Blob("Data");
    bt.insert("1122", value);
    CHECK(bt.find("1122", res));
    CHECK(value == res);
}
