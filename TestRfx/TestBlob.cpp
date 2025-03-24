
#include <gtest/gtest.h>
#include "Rfx/Blob.h"

using namespace Rfx;

TEST(Blob, DefaultCtor)
{
    Blob b;
    ASSERT_EQ(b.size(), 0);
    ASSERT_EQ(b.capacity(), 0);
    ASSERT_EQ(b.begin(), b.end());
}

TEST(Blob, sizeCtor)
{
    Blob b(1);
    ASSERT_EQ(b.size(), 1);
    ASSERT_GT(b.capacity(), b.size());
    ASSERT_EQ(b.begin() + b.size(), b.end());
}

TEST(Blob, initialize_list)
{
    Blob b = "test";
    ASSERT_EQ(b.size(), 4);
    auto ar = b.begin();
    ASSERT_EQ(ar[0], std::byte { 't' });
    ASSERT_EQ(ar[1], std::byte { 'e' });
    ASSERT_EQ(ar[2], std::byte { 's' });
    ASSERT_EQ(ar[3], std::byte { 't' });
}

TEST(Blob, CopyCtor)
{
    Blob b1 = "dasisntest";
    auto b2 = b1;
    ASSERT_EQ(b1.size(), b2.size());
    ASSERT_TRUE(std::equal(b1.begin(), b1.end(), b2.begin()));
    ASSERT_NE(b1.begin(), b2.begin());
}

TEST(Blob, Assignment)
{
    Blob b1 = "dasisntest";
    Blob b2;
    b2 = b1;
    ASSERT_EQ(b1.size(), b2.size());
    ASSERT_TRUE(std::equal(b1.begin(), b1.end(), b2.begin()));
    ASSERT_NE(b1.begin(), b2.begin());

    Blob b3 = "test";
    auto it = b3.begin();
    b3 = b1;
    ASSERT_EQ(b1.size(), b3.size());
    ASSERT_TRUE(std::equal(b1.begin(), b1.end(), b3.begin()));
    ASSERT_EQ(it, b3.begin());

    Blob b4 = b1;
    b4 = b4;
    ASSERT_EQ(b1.size(), b4.size());
    ASSERT_TRUE(std::equal(b1.begin(), b1.end(), b4.begin()));
}

TEST(Blob, comparison)
{
    Blob b1 = "testlang";
    Blob b2;
    ASSERT_NE(b1, b2);

    Blob b3 = b1;
    ASSERT_EQ(b1, b3);

    b2 = "test";
    ASSERT_LT(b2, b1);

    Blob b4(2000);
    b4.clear();
    b4 = b2;

    ASSERT_EQ(b4, b2);
    ASSERT_NE(b4.capacity(), b2.capacity());
}

TEST(Blob, moveCtorAssignment)
{
    Blob b1 = "dasisntest";
    {
        auto b2 = b1;
        auto it = b2.begin();
        auto b3 = std::move(b2);
        ASSERT_EQ(b3, b1);
        ASSERT_EQ(it, b3.begin());
    } // call moved-from d'tor

    auto b2 = b1;
    auto b3 = std::move(b2);
    auto it = b1.begin();
    b2 = std::move(b1); // move assign to a moved-from object
    ASSERT_EQ(b2, b3);
    ASSERT_EQ(it, b2.begin());

    b1 = b2; // assign to moved-from object
    ASSERT_EQ(b1, b2);
    ASSERT_NE(b1.begin(), b2.begin());
}

TEST(Blob, OtherCtorVariants)
{
    std::vector<std::byte> bv = { std::byte { 1 }, std::byte { 2 }, std::byte { 3 } };
    Blob b1(bv.begin(), bv.end());
    Blob b2(b1.crbegin(), b1.crend());

    std::ranges::reverse(b1);
    ASSERT_EQ(b1, b2);

    auto b3 = b2;
    b2.push_back(std::byte { 4 });
    ASSERT_NE(b2, b3);
}

TEST(Blob, clearResultsInZeroSize)
{
    Blob b = "DasIsnTest";
    b.clear();
    ASSERT_EQ(b.size(), 0);
}

TEST(Blob, capacityGrowsWhenNeeded)
{
    Blob b(1);
    auto oldCapacity = b.capacity();
    while (b.size() < b.capacity())
    {
        b.push_back({});
        ASSERT_EQ(oldCapacity, b.capacity());
    }

    ASSERT_EQ(b.size(), b.capacity());
    b.push_back({});
    ASSERT_NE(oldCapacity, b.capacity());
}

TEST(Blob, clearDoesntChangeCapacity)
{
    Blob b(10);
    size_t oldCapacity = b.capacity();
    b.clear();
    ASSERT_EQ(oldCapacity, b.capacity());
}

TEST(Blob, reserveDoesntChangeContentsButChangesBuffer)
{
    Blob b = "test";
    Blob b2 = b;
    auto it = b.begin();
    b.reserve(5000);
    ASSERT_EQ(b, b2);
    ASSERT_NE(it, b.begin());
}


