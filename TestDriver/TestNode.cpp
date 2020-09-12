

#include <gtest/gtest.h>
#include "CompoundFs/Leaf.h"
#include "CompoundFs/InnerNode.h"
#include <algorithm>
#include <random>

using namespace TxFs;

TEST(Node, hasPageSize)
{
    Leaf l;
    InnerNode n;

    ASSERT_EQ(sizeof(l) , 4096);
    ASSERT_EQ(sizeof(n) , 4096);
}

TEST(Leaf, insert)
{
    Leaf l;
    ASSERT_EQ(l.nofItems() , 0);
    size_t s = l.bytesLeft();

    l.insert("Test", "");
    ASSERT_EQ(l.nofItems() , 1);
    ASSERT_EQ(l.bytesLeft() , s - 8);

    l.insert("Anfang", "");
    ASSERT_EQ(l.nofItems() , 2);
    ASSERT_EQ(l.bytesLeft() , s - 18);
}

TEST(Leaf, keysAreSorted)
{
    Leaf l;
    std::vector<std::string> strs;
    for (int j = 900; j < 2000; j++)
        strs.push_back(std::to_string(j));

    std::shuffle(strs.begin(), strs.end(), std::mt19937(std::random_device()()));

    size_t i = 0;
    for (; i < strs.size(); i++)
    {
        if (!l.hasSpace(strs[i].c_str(), strs[i].c_str()))
            break;

        l.insert(strs[i].c_str(), strs[i].c_str());
        ASSERT_EQ(l.nofItems() , i + 1);
    }

    strs.resize(i);
    ASSERT_EQ(strs.size() , l.nofItems());

    std::sort(strs.begin(), strs.end());
    i = 0;
    for (uint16_t* it = l.beginTable(); it < l.endTable(); ++it)
    {
        ASSERT_EQ(l.getKey(it) , ByteString(strs[i++].c_str()));
    }
}

TEST(Leaf, find)
{
    Leaf l;

    l.insert("Test0", "Run");
    l.insert("Test1", "Run");
    l.insert("Test2", "Run");
    l.insert("Test3", "Run");

    ASSERT_NE(l.find("Test0") , l.endTable());
    ASSERT_NE(l.find("Test1") , l.endTable());
    ASSERT_NE(l.find("Test2") , l.endTable());
    ASSERT_NE(l.find("Test3") , l.endTable());

    ASSERT_EQ(l.find("Test") , l.endTable());
    ASSERT_EQ(l.find("Test11") , l.endTable());
    ASSERT_EQ(l.find("Test4") , l.endTable());
}

TEST(Leaf, removeInOppositeOrder)
{
    Leaf l;
    size_t free = l.bytesLeft();

    l.insert("Test0", "Run");
    l.insert("Test1", "Run");
    l.insert("Test2", "Run");
    l.insert("Test3", "Run");

    l.remove("Test3");
    ASSERT_EQ(l.nofItems() , 3);
    l.remove("Test2");
    ASSERT_EQ(l.nofItems() , 2);
    l.remove("Test1");
    ASSERT_EQ(l.nofItems() , 1);
    l.remove("Test0");
    ASSERT_EQ(l.nofItems() , 0);

    ASSERT_EQ(free , l.bytesLeft());
}

TEST(Leaf, removeInSameOrder)
{
    Leaf l;
    size_t free = l.bytesLeft();

    l.insert("Test0", "Run");
    l.insert("Test1", "Run");
    l.insert("Test2", "Run");
    l.insert("Test3", "Run");

    l.remove("Test0");
    ASSERT_EQ(l.nofItems() , 3);
    l.remove("Test1");
    ASSERT_EQ(l.nofItems() , 2);
    l.remove("Test2");
    ASSERT_EQ(l.nofItems() , 1);
    l.remove("Test3");
    ASSERT_EQ(l.nofItems() , 0);

    ASSERT_EQ(free , l.bytesLeft());
}

TEST(Leaf, removeWhatIsNotThere)
{
    Leaf l;

    l.insert("Test0", "Run");
    l.insert("Test1", "Run");
    l.insert("Test2", "Run");
    l.insert("Test3", "Run");

    l.remove("Test");
    l.remove("Test4");
    ASSERT_EQ(l.nofItems() , 4);
}

TEST(Leaf, removeInRandomOrder)
{
    Leaf l;
    size_t free = l.bytesLeft();

    std::vector<std::string> strs;
    for (int j = 0; j < 700; j++)
        strs.push_back(std::to_string(j));

    std::shuffle(strs.begin(), strs.end(), std::mt19937(std::random_device()()));

    size_t i = 0;
    for (; i < strs.size(); i++)
    {
        if (!l.hasSpace(strs[i].c_str(), strs[i].c_str()))
            break;

        l.insert(strs[i].c_str(), strs[i].c_str());
    }

    strs.resize(i);
    std::shuffle(strs.begin(), strs.end(), std::mt19937(std::random_device()()));

    for (auto & str : strs)
    {
        size_t items = l.nofItems();
        size_t bytesLeft = l.bytesLeft();
        l.remove(str.c_str());
        ASSERT_EQ(items , l.nofItems() + 1);
        ASSERT_TRUE(bytesLeft < l.bytesLeft());
    }

    ASSERT_EQ(free , l.bytesLeft());
}

TEST(Leaf, split)
{
    Leaf l;
    std::vector<std::string> strs;
    for (int j = 0; j < 500; j++)
        strs.push_back(std::to_string(j));

    std::shuffle(strs.begin(), strs.end(), std::mt19937(std::random_device()()));

    size_t i = 0;
    for (; i < strs.size(); i++)
    {
        if (!l.hasSpace(strs[i].c_str(), strs[i].c_str()))
            break;

        l.insert(strs[i].c_str(), strs[i].c_str());
    }

    strs.resize(i + 1);
    ASSERT_EQ(strs.size() - 1 , l.nofItems());

    Leaf m;
    l.split(&m, strs[i].c_str(), strs[i].c_str());

    std::sort(strs.begin(), strs.end());

    i = 0;
    for (uint16_t* it = l.beginTable(); it < l.endTable(); ++it)
    {
        ASSERT_EQ(l.getKey(it) , ByteString(strs[i++].c_str()));
    }

    for (uint16_t* it = m.beginTable(); it < m.endTable(); ++it)
    {
        ASSERT_EQ(m.getKey(it) , ByteString(strs[i++].c_str()));
    }
}

TEST(InnerNode, insertGrows)
{
    InnerNode m;
    ASSERT_EQ(m.nofItems() , 0);

    InnerNode n("100", 0, 100);
    ASSERT_EQ(n.nofItems() , 1);
    ASSERT_TRUE(m.bytesLeft() > n.bytesLeft());

    n.insert("200", 200);
    ASSERT_EQ(n.nofItems() , 2);
}

TEST(InnerNode, insertedItemsCanBeFound)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("300", 300);

    ASSERT_EQ(n.findPage("10") , 0);
    ASSERT_EQ(n.findPage("100") , 100);
    ASSERT_EQ(n.findPage("150") , 100);
    ASSERT_EQ(n.findPage("200") , 200);
    ASSERT_EQ(n.findPage("250") , 200);
    ASSERT_EQ(n.findPage("300") , 300);
    ASSERT_EQ(n.findPage("350") , 300);
    ASSERT_EQ(n.findPage("400") , 400);
    ASSERT_EQ(n.findPage("500") , 400);

    n.insert("150", 150);
    ASSERT_EQ(n.findPage("100") , 100);
    ASSERT_EQ(n.findPage("150") , 150);
    ASSERT_EQ(n.findPage("160") , 150);
    ASSERT_EQ(n.findPage("200") , 200);
}

TEST(InnerNode, leftAndRight)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("300", 300);

    uint16_t* it = n.beginTable();
    ASSERT_EQ(n.getLeft(it) , 0);
    ASSERT_EQ(n.getRight(it) , 100);
    ASSERT_EQ(n.getKey(it) , ByteString("100"));
    ++it;

    ASSERT_EQ(n.getLeft(it) , 100);
    ASSERT_EQ(n.getRight(it) , 200);
    ASSERT_EQ(n.getKey(it) , ByteString("200"));
    ++it;

    ASSERT_EQ(n.getLeft(it) , 200);
    ASSERT_EQ(n.getRight(it) , 300);
    ASSERT_EQ(n.getKey(it) , ByteString("300"));
    ++it;

    ASSERT_EQ(n.getLeft(it) , 300);
    ASSERT_EQ(n.getRight(it) , 400);
    ASSERT_EQ(n.getKey(it) , ByteString("400"));
    ++it;

    ASSERT_EQ(it , n.endTable());
    ASSERT_EQ(n.getLeft(it) , 400);
}

TEST(InnerNode, split)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("500", 500);
    n.insert("300", 300);

    InnerNode right;
    ByteString key = n.split(&right);

    ASSERT_EQ(n.nofItems() , 2);
    ASSERT_EQ(right.nofItems() , 2);
    ASSERT_EQ(key , ByteString("300"));

    uint16_t* it = n.beginTable();
    ASSERT_EQ(n.getLeft(it) , 0);
    ASSERT_EQ(n.getRight(it) , 100);
    ++it;
    ASSERT_EQ(n.getLeft(it) , 100);
    ASSERT_EQ(n.getRight(it) , 200);
    ++it;
    ASSERT_EQ(it , n.endTable());

    it = right.beginTable();
    ASSERT_EQ(right.getLeft(it) , 300);
    ASSERT_EQ(right.getRight(it) , 400);
    ++it;
    ASSERT_EQ(right.getLeft(it) , 400);
    ASSERT_EQ(right.getRight(it) , 500);
    ++it;
    ASSERT_EQ(it , right.endTable());
}

TEST(InnerNode, splitInsertsTheNewKeyLeft)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("500", 500);
    n.insert("300", 300);

    InnerNode right;
    ByteString key = n.split(&right, ByteString("250"), 250);

    ASSERT_EQ(key , ByteString("300"));
    ASSERT_EQ(n.nofItems() , 3);
    ASSERT_EQ(right.nofItems() , 2);
    auto it = n.endTable() - 1;
    ASSERT_EQ(n.getRight(it) , 250);
    ASSERT_EQ(n.getLeft(n.beginTable()) , 0);
    ASSERT_EQ(right.getLeft(right.beginTable()) , 300);
}
TEST(InnerNode, splitInsertsTheNewKeyRight)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("500", 500);
    n.insert("300", 300);

    InnerNode right;
    ByteString key = n.split(&right, ByteString("350"), 350);

    ASSERT_EQ(key , ByteString("300"));
    ASSERT_EQ(n.nofItems() , 2);
    ASSERT_EQ(right.nofItems() , 3);
    auto it = right.beginTable();
    ASSERT_EQ(right.getRight(it) , 350);
    ASSERT_EQ(right.getLeft(it) , 300);
}

InnerNode createInnerNode(const std::vector<int>& keys)
{
    InnerNode n(std::to_string(keys[0]).c_str(), 0, keys[0]);
    for (size_t i = 1; i < keys.size(); i++)
        n.insert(std::to_string(keys[i]).c_str(), keys[i]);
    return n;
}

void removeInnerNode(InnerNode& n, const std::vector<int>& keys)
{
    for (size_t i = 0; i < keys.size(); i++)
        n.remove(std::to_string(keys[i]).c_str());
}

std::vector<int> createKeys(int begin, int end, int step)
{
    std::vector<int> keys;
    for (int i = begin; i < end; i += step)
        keys.push_back(i);
    return keys;
}

TEST(InnerNode, remove)
{
    std::vector<int> keys = createKeys(10, 300, 10);
    InnerNode n = createInnerNode(keys);
    removeInnerNode(n, keys);
    ASSERT_EQ(n.nofItems() , 0);

    n = createInnerNode(keys);
    std::reverse(keys.begin(), keys.end());
    removeInnerNode(n, keys);
    ASSERT_EQ(n.nofItems() , 0);

    n = createInnerNode(keys);
    removeInnerNode(n, keys);
    ASSERT_EQ(n.nofItems() , 0);

    n = createInnerNode(keys);
    std::reverse(keys.begin(), keys.end());
    removeInnerNode(n, keys);
    ASSERT_EQ(n.nofItems() , 0);

    n = createInnerNode(keys);
    std::shuffle(keys.begin(), keys.end(), std::mt19937(std::random_device()()));
    removeInnerNode(n, keys);
    ASSERT_EQ(n.nofItems() , 0);

    n = createInnerNode(keys);
    removeInnerNode(n, keys);
    ASSERT_EQ(n.nofItems() , 0);
}

TEST(InnerNode, removeDetail)
{
    std::vector<int> keys = createKeys(20, 70, 10);
    InnerNode n = createInnerNode(keys);

    PageIndex page = n.findPage("35");
    n.remove("35");
    ASSERT_TRUE(n.findPage("35") < page);

    page = n.findPage("99");
    n.remove("99");
    ASSERT_TRUE(n.findPage("99") < page);

    page = n.findPage("1");
    n.remove("1");
    ASSERT_TRUE(n.findPage("1") > page);
}

TEST(InnerNode, copyToBack)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("500", 500);
    n.insert("300", 300);

    InnerNode m("600", 550, 600);
    m.insert("800", 800);
    m.insert("700", 700);

    n.copyToBack(m, m.beginTable(), m.beginTable());
    ASSERT_EQ(n.nofItems() , 5);

    n.copyToBack(m, m.beginTable(), m.beginTable() + 1);
    ASSERT_EQ(n.nofItems() , 6);
    ASSERT_EQ(n.findPage(ByteString("600")) , 600);

    n.copyToBack(m, m.beginTable() + 1, m.endTable());
    ASSERT_EQ(n.nofItems() , 8);
    ASSERT_EQ(n.findPage(ByteString("600")) , 600);
    ASSERT_EQ(n.findPage(ByteString("700")) , 700);
    ASSERT_EQ(n.findPage(ByteString("800")) , 800);
}

TEST(InnerNode, copyToFront)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("500", 500);
    n.insert("300", 300);

    InnerNode m("600", 550, 600);
    m.insert("800", 800);
    m.insert("700", 700);

    m.copyToFront(n, n.beginTable(), n.beginTable());
    ASSERT_EQ(m.nofItems() , 3);

    m.copyToFront(n, n.endTable() - 1, n.endTable());
    ASSERT_EQ(m.nofItems() , 4);
    ASSERT_EQ(m.findPage(ByteString("500")) , 500);
    ASSERT_EQ(m.getLeft(m.beginTable()) , 400);

    m.copyToFront(n, n.beginTable(), n.endTable() - 1);
    ASSERT_EQ(m.nofItems() , 8);
    uint32_t pidx = 100;
    for (auto it = m.beginTable(); it < m.endTable(); ++it)
    {
        ASSERT_EQ(m.getRight(it) , pidx);
        pidx += 100;
    }

    ASSERT_EQ(m.getLeft(m.beginTable()) , 0);
}

TEST(InnerNode, removeLeftMost)
{
    InnerNode n("100", 0, 100);
    n.insert("200", 200);

    n.remove(ByteString("000"));
    ASSERT_EQ(n.getLeft(n.beginTable()) , 100);
    ASSERT_EQ(n.getRight(n.beginTable()) , 200);
}

TEST(InnerNode, removeRightOfFirstNode)
{
    InnerNode n("100", 0, 100);
    n.insert("200", 200);

    n.remove(ByteString("100"));
    ASSERT_EQ(n.getLeft(n.beginTable()) , 0);
    ASSERT_EQ(n.getRight(n.beginTable()) , 200);
}

TEST(InnerNode, removeRightMost)
{
    InnerNode n("100", 0, 100);
    n.insert("200", 200);

    n.remove(ByteString("300"));
    ASSERT_EQ(n.getLeft(n.beginTable()) , 0);
    ASSERT_EQ(n.getRight(n.beginTable()) , 100);
    ASSERT_EQ(n.nofItems() , 1);
}

TEST(InnerNode, removeSingleEntryRight)
{
    InnerNode n("100", 0, 100);

    n.remove(ByteString("300"));
    ASSERT_EQ(n.getLeft(n.beginTable()) , 0);
    ASSERT_EQ(n.nofItems() , 0);
}

TEST(InnerNode, removeSingleEntryLeft)
{
    InnerNode n("100", 0, 100);

    n.remove(ByteString("050"));
    ASSERT_EQ(n.getLeft(n.beginTable()) , 100);
    ASSERT_EQ(n.nofItems() , 0);
}

TEST(InnerNode, replaceSingleKeepsSingleEntryCorrect)
{
    InnerNode n("100", 0, 100);

    n.remove(ByteString("100"));
    n.insert("075", 100);
    ASSERT_EQ(n.getLeft(n.beginTable()) , 0);
    ASSERT_EQ(n.getRight(n.beginTable()) , 100);
}

TEST(InnerNode, redistributeLeftPageBigger)
{
    InnerNode n("100", 0, 100);
    n.insert("400", 400);
    n.insert("200", 200);
    n.insert("300", 300);
    InnerNode m("500", 450, 500);

    ByteString key("450");
    key = InnerNode::redistribute(n, m, key);
    ASSERT_EQ(key , ByteString("300"));
    ASSERT_EQ(n.nofItems() , 2);
    ASSERT_EQ(m.nofItems() , 3);

    ASSERT_EQ(n.getLeft(n.beginTable()) , 0);
    ASSERT_EQ(n.getRight(n.beginTable()) , 100);
    ASSERT_EQ(n.getRight(n.beginTable() + 1) , 200);

    ASSERT_EQ(m.getLeft(m.beginTable()) , 300);
    ASSERT_EQ(m.getRight(m.beginTable()) , 400);
    ASSERT_EQ(m.getRight(m.beginTable() + 1) , 450);
    ASSERT_EQ(m.getRight(m.beginTable() + 2) , 500);
}

TEST(InnerNode, redistributeLeftPageSmaller)
{
    InnerNode n("100", 0, 100);
    InnerNode m("200", 150, 200);
    m.insert("400", 400);
    m.insert("300", 300);
    m.insert("500", 500);

    ByteString key("150");
    key = InnerNode::redistribute(n, m, key);
    ASSERT_EQ(key , ByteString("300"));
    ASSERT_EQ(n.nofItems() , 3);
    ASSERT_EQ(m.nofItems() , 2);

    ASSERT_EQ(n.getLeft(n.beginTable()) , 0);
    ASSERT_EQ(n.getRight(n.beginTable()) , 100);
    ASSERT_EQ(n.getRight(n.beginTable() + 1) , 150);
    ASSERT_EQ(n.getRight(n.beginTable() + 2) , 200);

    ASSERT_EQ(m.getLeft(m.beginTable()) , 300);
    ASSERT_EQ(m.getRight(m.beginTable()) , 400);
    ASSERT_EQ(m.getRight(m.beginTable() + 1) , 500);
}

TEST(InnerNode, mergeWithRightPage)
{
    InnerNode n("100", 0, 100);
    InnerNode m("200", 150, 200);
    m.insert("400", 400);
    m.insert("300", 300);
    m.insert("500", 500);

    ByteString key("150");
    n.mergeWith(m, key);
    ASSERT_EQ(n.nofItems() , 6);

    ASSERT_EQ(n.getLeft(n.beginTable()) , 0);
    ASSERT_EQ(n.getRight(n.beginTable()) , 100);
    ASSERT_EQ(n.getRight(n.beginTable() + 1) , 150);
    ASSERT_EQ(n.getRight(n.beginTable() + 2) , 200);
    ASSERT_EQ(n.getRight(n.beginTable() + 3) , 300);
    ASSERT_EQ(n.getRight(n.beginTable() + 4) , 400);
    ASSERT_EQ(n.getRight(n.beginTable() + 5) , 500);
}
