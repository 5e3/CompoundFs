
#include "stdafx.h"
#include "Test.h"
#include "Leaf.h"
#include "InnerNode.h"

using namespace TxFs;

TEST(Blob, Ctor)
{
    Blob b("Nils");
    CHECK(b.size() == 5);

    Blob c("Senta");
    CHECK(!(b == c));
    CHECK(b == b);

    Blob d = c;
    CHECK(d == c);
    CHECK(d.begin() != c.begin());

    b = d;
    CHECK(b == c);
    CHECK(b.begin() != c.begin());

    Blob e(b.begin());
    CHECK(e == b);
    CHECK(e.begin() == b.begin());
}

TEST(Node, hasPageSize)
{
    Leaf l;
    InnerNode n;

    CHECK(sizeof(l) == 4096);
    CHECK(sizeof(n) == 4096);
}

TEST(Leaf, insert)
{
    Leaf l;
    CHECK(l.itemSize() == 0);
    size_t s = l.bytesLeft();

    l.insert("Test", "");
    CHECK(l.itemSize() == 1);
    CHECK(l.bytesLeft() == s - 8);

    l.insert("Anfang", "");
    CHECK(l.itemSize() == 2);
    CHECK(l.bytesLeft() == s - 18);
}

TEST(Leaf, keysAreSorted)
{
    Leaf l;
    std::vector<std::string> strs;
    for (int j = 900; j < 2000; j++)
        strs.push_back(std::to_string(j));

    std::random_shuffle(strs.begin(), strs.end());

    size_t i = 0;
    for (; i < strs.size(); i++)
    {
        if (!l.hasSpace(strs[i].c_str(), strs[i].c_str()))
            break;

        l.insert(strs[i].c_str(), strs[i].c_str());
        CHECK(l.itemSize() == i + 1);
    }

    strs.resize(i);
    CHECK(strs.size() == l.itemSize());

    std::sort(strs.begin(), strs.end());
    i = 0;
    for (uint16_t* it = l.beginTable(); it < l.endTable(); ++it)
    {
        CHECK(l.getKey(it) == Blob(strs[i++].c_str()));
    }
}

TEST(Leaf, find)
{
    Leaf l;

    l.insert("Test0", "Run");
    l.insert("Test1", "Run");
    l.insert("Test2", "Run");
    l.insert("Test3", "Run");

    CHECK(l.find("Test0") != l.endTable());
    CHECK(l.find("Test1") != l.endTable());
    CHECK(l.find("Test2") != l.endTable());
    CHECK(l.find("Test3") != l.endTable());

    CHECK(l.find("Test") == l.endTable());
    CHECK(l.find("Test11") == l.endTable());
    CHECK(l.find("Test4") == l.endTable());
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
    CHECK(l.itemSize() == 3);
    l.remove("Test2");
    CHECK(l.itemSize() == 2);
    l.remove("Test1");
    CHECK(l.itemSize() == 1);
    l.remove("Test0");
    CHECK(l.itemSize() == 0);

    CHECK(free == l.bytesLeft());
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
    CHECK(l.itemSize() == 3);
    l.remove("Test1");
    CHECK(l.itemSize() == 2);
    l.remove("Test2");
    CHECK(l.itemSize() == 1);
    l.remove("Test3");
    CHECK(l.itemSize() == 0);

    CHECK(free == l.bytesLeft());
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
    CHECK(l.itemSize() == 4);
}

TEST(Leaf, removeInRandomOrder)
{
    Leaf l;
    size_t free = l.bytesLeft();

    std::vector<std::string> strs;
    for (int j = 0; j < 700; j++)
        strs.push_back(std::to_string(j));

    std::random_shuffle(strs.begin(), strs.end());

    size_t i = 0;
    for (; i < strs.size(); i++)
    {
        if (!l.hasSpace(strs[i].c_str(), strs[i].c_str()))
            break;

        l.insert(strs[i].c_str(), strs[i].c_str());
    }

    strs.resize(i);
    std::random_shuffle(strs.begin(), strs.end());

    for (size_t i = 0; i < strs.size(); i++)
    {
        size_t items = l.itemSize();
        size_t bytesLeft = l.bytesLeft();
        l.remove(strs[i].c_str());
        CHECK(items == l.itemSize() + 1);
        CHECK(bytesLeft < l.bytesLeft());
    }

    CHECK(free == l.bytesLeft());
}

TEST(Leaf, split)
{
    Leaf l;
    std::vector<std::string> strs;
    for (int j = 0; j < 500; j++)
        strs.push_back(std::to_string(j));

    std::random_shuffle(strs.begin(), strs.end());

    size_t i = 0;
    for (; i < strs.size(); i++)
    {
        if (!l.hasSpace(strs[i].c_str(), strs[i].c_str()))
            break;

        l.insert(strs[i].c_str(), strs[i].c_str());
    }

    strs.resize(i + 1);
    CHECK(strs.size() - 1 == l.itemSize());

    Leaf m;
    l.split(&m, strs[i].c_str(), strs[i].c_str());

    std::sort(strs.begin(), strs.end());

    i = 0;
    for (uint16_t* it = l.beginTable(); it < l.endTable(); ++it)
    {
        CHECK(l.getKey(it) == Blob(strs[i++].c_str()));
    }

    for (uint16_t* it = m.beginTable(); it < m.endTable(); ++it)
    {
        CHECK(m.getKey(it) == Blob(strs[i++].c_str()));
    }
}

TEST(InnerNode, insertGrows)
{
    InnerNode m;
    CHECK(m.itemSize() == 0);

    InnerNode n("100", 0, 100);
    CHECK(n.itemSize() == 1);
    CHECK(m.bytesLeft() > n.bytesLeft());

    n.insert("200", 200);
    CHECK(n.itemSize() == 2);
}

TEST(InnerNode, insertedItemsCanBeFound)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("300", 300);

    CHECK(n.findPage("10") == 0);
    CHECK(n.findPage("100") == 100);
    CHECK(n.findPage("150") == 100);
    CHECK(n.findPage("200") == 200);
    CHECK(n.findPage("250") == 200);
    CHECK(n.findPage("300") == 300);
    CHECK(n.findPage("350") == 300);
    CHECK(n.findPage("400") == 400);
    CHECK(n.findPage("500") == 400);

    n.insert("150", 150);
    CHECK(n.findPage("100") == 100);
    CHECK(n.findPage("150") == 150);
    CHECK(n.findPage("160") == 150);
    CHECK(n.findPage("200") == 200);
}

TEST(InnerNode, leftAndRight)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("300", 300);

    uint16_t* it = n.beginTable();
    CHECK(n.getLeft(it) == 0);
    CHECK(n.getRight(it) == 100);
    CHECK(n.getKey(it) == Blob("100"));
    ++it;

    CHECK(n.getLeft(it) == 100);
    CHECK(n.getRight(it) == 200);
    CHECK(n.getKey(it) == Blob("200"));
    ++it;

    CHECK(n.getLeft(it) == 200);
    CHECK(n.getRight(it) == 300);
    CHECK(n.getKey(it) == Blob("300"));
    ++it;

    CHECK(n.getLeft(it) == 300);
    CHECK(n.getRight(it) == 400);
    CHECK(n.getKey(it) == Blob("400"));
    ++it;

    CHECK(it == n.endTable());
    CHECK(n.getLeft(it) == 400);
}

TEST(InnerNode, split)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("500", 500);
    n.insert("300", 300);

    InnerNode right;
    Blob key;
    n.split(&right, key);

    CHECK(n.itemSize() == 2);
    CHECK(right.itemSize() == 2);
    CHECK(key == Blob("300"));

    uint16_t* it = n.beginTable();
    CHECK(n.getLeft(it) == 0);
    CHECK(n.getRight(it) == 100);
    ++it;
    CHECK(n.getLeft(it) == 100);
    CHECK(n.getRight(it) == 200);
    ++it;
    CHECK(it == n.endTable());

    it = right.beginTable();
    CHECK(right.getLeft(it) == 300);
    CHECK(right.getRight(it) == 400);
    ++it;
    CHECK(right.getLeft(it) == 400);
    CHECK(right.getRight(it) == 500);
    ++it;
    CHECK(it == right.endTable());
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
    CHECK(n.itemSize() == 0);

    n = createInnerNode(keys);
    std::reverse(keys.begin(), keys.end());
    removeInnerNode(n, keys);
    CHECK(n.itemSize() == 0);

    n = createInnerNode(keys);
    removeInnerNode(n, keys);
    CHECK(n.itemSize() == 0);

    n = createInnerNode(keys);
    std::reverse(keys.begin(), keys.end());
    removeInnerNode(n, keys);
    CHECK(n.itemSize() == 0);

    n = createInnerNode(keys);
    std::random_shuffle(keys.begin(), keys.end());
    removeInnerNode(n, keys);
    CHECK(n.itemSize() == 0);

    n = createInnerNode(keys);
    removeInnerNode(n, keys);
    CHECK(n.itemSize() == 0);
}

TEST(InnerNode, removeDetail)
{
    std::vector<int> keys = createKeys(20, 70, 10);
    InnerNode n = createInnerNode(keys);

    Node::Id page = n.findPage("35");
    n.remove("35");
    CHECK(n.findPage("35") < page);

    page = n.findPage("99");
    n.remove("99");
    CHECK(n.findPage("99") < page);

    page = n.findPage("1");
    n.remove("1");
    CHECK(n.findPage("1") > page);
}
