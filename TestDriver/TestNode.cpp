
#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/Leaf.h"
#include "../CompoundFs/InnerNode.h"

using namespace TxFs;

TEST(Blob, Ctor)
{
    Blob a;
    CHECK(a.size() == 1);

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
    CHECK(e.begin() != b.begin());
}

TEST(BlobRef, Transformations)
{
    Blob b("Senta");
    BlobRef br = b;
    CHECK(br.begin() == b.begin());

    Blob c(std::move(b));
    CHECK(br.begin() == c.begin());

    b = br; // reassignment after move is legal
    CHECK(br.begin() != b.begin());
    CHECK(br == b);

    br = b;
    c = std::move(b);
    CHECK(br.begin() == c.begin());
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
    CHECK(l.nofItems() == 0);
    size_t s = l.bytesLeft();

    l.insert("Test", "");
    CHECK(l.nofItems() == 1);
    CHECK(l.bytesLeft() == s - 8);

    l.insert("Anfang", "");
    CHECK(l.nofItems() == 2);
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
        CHECK(l.nofItems() == i + 1);
    }

    strs.resize(i);
    CHECK(strs.size() == l.nofItems());

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
    CHECK(l.nofItems() == 3);
    l.remove("Test2");
    CHECK(l.nofItems() == 2);
    l.remove("Test1");
    CHECK(l.nofItems() == 1);
    l.remove("Test0");
    CHECK(l.nofItems() == 0);

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
    CHECK(l.nofItems() == 3);
    l.remove("Test1");
    CHECK(l.nofItems() == 2);
    l.remove("Test2");
    CHECK(l.nofItems() == 1);
    l.remove("Test3");
    CHECK(l.nofItems() == 0);

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
    CHECK(l.nofItems() == 4);
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
        size_t items = l.nofItems();
        size_t bytesLeft = l.bytesLeft();
        l.remove(strs[i].c_str());
        CHECK(items == l.nofItems() + 1);
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
    CHECK(strs.size() - 1 == l.nofItems());

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
    CHECK(m.nofItems() == 0);

    InnerNode n("100", 0, 100);
    CHECK(n.nofItems() == 1);
    CHECK(m.bytesLeft() > n.bytesLeft());

    n.insert("200", 200);
    CHECK(n.nofItems() == 2);
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
    Blob key = n.split(&right);

    CHECK(n.nofItems() == 2);
    CHECK(right.nofItems() == 2);
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

TEST(InnerNode, splitInsertsTheNewKeyLeft)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("500", 500);
    n.insert("300", 300);

    InnerNode right;
    Blob key = n.split(&right, Blob("250"), 250);

    CHECK(key == Blob("300"));
    CHECK(n.nofItems() == 3);
    CHECK(right.nofItems() == 2);
    auto it = n.endTable() - 1;
    CHECK(n.getRight(it) == 250);
    CHECK(n.getLeft(n.beginTable()) == 0);
    CHECK(right.getLeft(right.beginTable()) == 300);
}
TEST(InnerNode, splitInsertsTheNewKeyRight)
{
    InnerNode n("200", 0, 200);
    n.insert("100", 100);
    n.insert("400", 400);
    n.insert("500", 500);
    n.insert("300", 300);

    InnerNode right;
    Blob key = n.split(&right, Blob("350"), 350);

    CHECK(key == Blob("300"));
    CHECK(n.nofItems() == 2);
    CHECK(right.nofItems() == 3);
    auto it = right.beginTable();
    CHECK(right.getRight(it) == 350);
    CHECK(right.getLeft(it) == 300);
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
    CHECK(n.nofItems() == 0);

    n = createInnerNode(keys);
    std::reverse(keys.begin(), keys.end());
    removeInnerNode(n, keys);
    CHECK(n.nofItems() == 0);

    n = createInnerNode(keys);
    removeInnerNode(n, keys);
    CHECK(n.nofItems() == 0);

    n = createInnerNode(keys);
    std::reverse(keys.begin(), keys.end());
    removeInnerNode(n, keys);
    CHECK(n.nofItems() == 0);

    n = createInnerNode(keys);
    std::random_shuffle(keys.begin(), keys.end());
    removeInnerNode(n, keys);
    CHECK(n.nofItems() == 0);

    n = createInnerNode(keys);
    removeInnerNode(n, keys);
    CHECK(n.nofItems() == 0);
}

TEST(InnerNode, removeDetail)
{
    std::vector<int> keys = createKeys(20, 70, 10);
    InnerNode n = createInnerNode(keys);

    PageIndex page = n.findPage("35");
    n.remove("35");
    CHECK(n.findPage("35") < page);

    page = n.findPage("99");
    n.remove("99");
    CHECK(n.findPage("99") < page);

    page = n.findPage("1");
    n.remove("1");
    CHECK(n.findPage("1") > page);
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
    CHECK(n.nofItems() == 5);

    n.copyToBack(m, m.beginTable(), m.beginTable() + 1);
    CHECK(n.nofItems() == 6);
    CHECK(n.findPage(Blob("600")) == 600);

    n.copyToBack(m, m.beginTable() + 1, m.endTable());
    CHECK(n.nofItems() == 8);
    CHECK(n.findPage(Blob("600")) == 600);
    CHECK(n.findPage(Blob("700")) == 700);
    CHECK(n.findPage(Blob("800")) == 800);
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
    CHECK(m.nofItems() == 3);

    m.copyToFront(n, n.endTable() - 1, n.endTable());
    CHECK(m.nofItems() == 4);
    CHECK(m.findPage(Blob("500")) == 500);
    CHECK(m.getLeft(m.beginTable()) == 400);

    m.copyToFront(n, n.beginTable(), n.endTable() - 1);
    CHECK(m.nofItems() == 8);
    uint32_t pidx = 100;
    for (auto it = m.beginTable(); it < m.endTable(); ++it)
    {
        CHECK(m.getRight(it) == pidx);
        pidx += 100;
    }

    CHECK(m.getLeft(m.beginTable()) == 0);
}

TEST(InnerNode, removeLeftMost)
{
    InnerNode n("100", 0, 100);
    n.insert("200", 200);

    n.remove(Blob("000"));
    CHECK(n.getLeft(n.beginTable()) == 100);
    CHECK(n.getRight(n.beginTable()) == 200);
}

TEST(InnerNode, removeRightOfFirstNode)
{
    InnerNode n("100", 0, 100);
    n.insert("200", 200);

    n.remove(Blob("100"));
    CHECK(n.getLeft(n.beginTable()) == 0);
    CHECK(n.getRight(n.beginTable()) == 200);
}

TEST(InnerNode, removeRightMost)
{
    InnerNode n("100", 0, 100);
    n.insert("200", 200);

    n.remove(Blob("300"));
    CHECK(n.getLeft(n.beginTable()) == 0);
    CHECK(n.getRight(n.beginTable()) == 100);
    CHECK(n.nofItems() == 1);
}

TEST(InnerNode, removeSingleEntryRight)
{
    InnerNode n("100", 0, 100);

    n.remove(Blob("300"));
    CHECK(n.getLeft(n.beginTable()) == 0);
    CHECK(n.nofItems() == 0);
}

TEST(InnerNode, removeSingleEntryLeft)
{
    InnerNode n("100", 0, 100);

    n.remove(Blob("050"));
    CHECK(n.getLeft(n.beginTable()) == 100);
    CHECK(n.nofItems() == 0);
}

TEST(InnerNode, replaceSingleKeepsSingleEntryCorrect)
{
    InnerNode n("100", 0, 100);

    n.remove(Blob("100"));
    n.insert("075", 100);
    CHECK(n.getLeft(n.beginTable()) == 0);
    CHECK(n.getRight(n.beginTable()) == 100);
}

TEST(InnerNode, redistributeLeftPageBigger)
{
    InnerNode n("100", 0, 100);
    n.insert("400", 400);
    n.insert("200", 200);
    n.insert("300", 300);
    InnerNode m("500", 450, 500);

    Blob key("450");
    key = InnerNode::redistribute(n, m, key);
    CHECK(key == Blob("300"));
    CHECK(n.nofItems() == 2);
    CHECK(m.nofItems() == 3);

    CHECK(n.getLeft(n.beginTable()) == 0);
    CHECK(n.getRight(n.beginTable()) == 100);
    CHECK(n.getRight(n.beginTable() + 1) == 200);

    CHECK(m.getLeft(m.beginTable()) == 300);
    CHECK(m.getRight(m.beginTable()) == 400);
    CHECK(m.getRight(m.beginTable() + 1) == 450);
    CHECK(m.getRight(m.beginTable() + 2) == 500);
}

TEST(InnerNode, redistributeLeftPageSmaller)
{
    InnerNode n("100", 0, 100);
    InnerNode m("200", 150, 200);
    m.insert("400", 400);
    m.insert("300", 300);
    m.insert("500", 500);

    Blob key("150");
    key = InnerNode::redistribute(n, m, key);
    CHECK(key == Blob("300"));
    CHECK(n.nofItems() == 3);
    CHECK(m.nofItems() == 2);

    CHECK(n.getLeft(n.beginTable()) == 0);
    CHECK(n.getRight(n.beginTable()) == 100);
    CHECK(n.getRight(n.beginTable() + 1) == 150);
    CHECK(n.getRight(n.beginTable() + 2) == 200);

    CHECK(m.getLeft(m.beginTable()) == 300);
    CHECK(m.getRight(m.beginTable()) == 400);
    CHECK(m.getRight(m.beginTable() + 1) == 500);
}

TEST(InnerNode, mergeWithRightPage)
{
    InnerNode n("100", 0, 100);
    InnerNode m("200", 150, 200);
    m.insert("400", 400);
    m.insert("300", 300);
    m.insert("500", 500);

    Blob key("150");
    n.mergeWidth(m, key);
    CHECK(n.nofItems() == 6);

    CHECK(n.getLeft(n.beginTable()) == 0);
    CHECK(n.getRight(n.beginTable()) == 100);
    CHECK(n.getRight(n.beginTable()+1) == 150);
    CHECK(n.getRight(n.beginTable() + 2) == 200);
    CHECK(n.getRight(n.beginTable() + 3) == 300);
    CHECK(n.getRight(n.beginTable() + 4) == 400);
    CHECK(n.getRight(n.beginTable() + 5) == 500);
}
