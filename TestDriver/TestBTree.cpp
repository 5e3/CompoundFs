

#include "stdafx.h"
#include "Test.h"
#include "BTree.h"
#include "Blob.h"

using namespace CompFs;

#ifdef _DEBUG
#define MANYITERATION 3000
#else
#define MANYITERATION 200000
#endif

TEST(BTree, insert)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    BTree bt(pm);

    for (size_t i = 0; i < MANYITERATION; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), "");
    }

    Blob value;
    for (size_t i = 0; i < MANYITERATION; i++)
        CHECK(bt.find(std::to_string(i).c_str(), value));

}

TEST(BTree, insertReplacesOriginal)
{
    std::shared_ptr<PageManager> pm(new PageManager);
    BTree bt(pm);

    for (size_t i = 0; i < 100; i++)
    {
        std::string s = std::to_string(i);
        bt.insert(s.c_str(), "TestData");
    }

    // value has same size => inplace
    Blob value("Te$tData");
    Blob res;
    bt.insert("33", value);
    CHECK(bt.find("33", res));
    CHECK(value == res);

    //value has different size => remove, add
    value = Blob("Data");
    bt.insert("22", value);
    CHECK(bt.find("22", res));
    CHECK(value == res);
}

