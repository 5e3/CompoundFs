
#include <gtest/gtest.h>
#include "Rfx/PushBits.h"
#include "Rfx/Blob.h"
#include <array>
#include <deque>
#include <list>
#include <exception>

using namespace Rfx;

TEST(PushBits, copyBits)
{
    double d = 3.14;
    Blob b(100);
    auto it = copyBits(d, b.begin());

    ASSERT_EQ(it - b.begin(), sizeof(double));

    enum class Test : short{};
    Test t {};
    it = copyBits(t, it);

    ASSERT_EQ(it - b.begin(), sizeof(double)+sizeof(short));

    Blob b2(sizeof(double));
    std::vector<std::byte> bv(sizeof(double));

    copyBits(d, bv.rbegin());
    copyBits(d, b2.begin());
    ASSERT_EQ(b2, Blob(bv.rbegin(),bv.rend()));

    // make a palindrome
    b2.grow(sizeof(double));
    copyBits(d, b2.rbegin());
    for (auto i = 0; i < sizeof(double); ++i)
        ASSERT_EQ(b2.crbegin()[i], b2.cbegin()[i]);
 }

TEST(PushBits, copyBitsBack)
{
    Blob b(2*sizeof(double));
    double in1 = 3.14;
    double in2 = 2.2;

    auto it = copyBits(in1, b.begin());
    copyBits(in2, it);

    double out1,out2;
    auto it2 = copyBits(b.begin(), out1);
    copyBits(it2, out2);
    ASSERT_EQ(in1, out1);
    ASSERT_EQ(in2, out2);
}

template<typename TCont>
void testCopyBitsForRandomAccessContainers(const TCont& cont)
{
    Blob b(3 * sizeof(double));

    copyBits(cont.begin(), cont.end(), b.begin());
    double d;

    auto it = copyBits(b.begin(), d);
    ASSERT_EQ(d, cont[0]);
    it = copyBits(it, d);
    ASSERT_EQ(d, cont[1]);
    copyBits(it, d);
    ASSERT_EQ(d, cont[2]);
}

TEST(PushBits, copyBitsForContiguousRanges)
{
    std::array<double, 3> ar = { 1.1, 2.2, 3.3 };
    testCopyBitsForRandomAccessContainers(ar);
}

TEST(PushBits, copyBitsForGeneralRanges)
{
    std::deque<double> dq = { 1.111, 2.222, 3.333 };
    testCopyBitsForRandomAccessContainers(dq);
}

template<typename TCont>
void testCopyBitsBack(const TCont& cont)
{
    Blob b(cont.size() * sizeof(typename TCont::value_type));
    copyBits(cont.begin(), cont.end(), b.begin());

    TCont cont2;
    cont2.resize(cont.size());
    copyBits(b.begin(), cont2.begin(), cont2.end());
    ASSERT_EQ(cont, cont2);
}

TEST(PushBits, copyBitsBackFromGeneralRanges)
{
    std::deque<double> dq = { 1.111, 2.222, 3.333 };
    testCopyBitsBack(dq);
}

TEST(PushBits, copyBitsBackFromContiguousRanges)
{
    std::vector<double> vec = { 1.111, 2.222, 3.333 };
    testCopyBitsBack(vec);
}

namespace
{
// not a std::sized_range if used with a e.g. std::list
template <typename T>
struct SimpleRange 
{
    T m_range;

    auto begin() { return m_range.begin(); }
    auto end() { return m_range.end(); }
    auto begin()const { return m_range.begin(); }
    auto end()const { return m_range.end(); }

    SimpleRange() = default;
    SimpleRange(T range)
        : m_range(std::move(range))
    {
    }
    void resize(size_t size) { m_range.resize(size); }
    bool operator==(const SimpleRange& rhs) const { return m_range == rhs.m_range; }
};

template <typename TCont>
void testPushBits(const TCont& cont, size_t size=0)
{
    Blob b;
    pushBits(cont, b);

    TCont cont2;
    if constexpr (std::ranges::sized_range<TCont>)
        cont2.resize(std::ranges::size(cont));
    else
        cont2.resize(size);
    copyBits(b.begin(), cont2.begin(), cont2.end());
    ASSERT_EQ(cont, cont2);
}
}
TEST(PushBits, pushBitsRange)
{
    std::deque<double> dq = { 1.111, 2.222, 3.333 };
    testPushBits(dq);

    std::vector<double> vec = { 1.111, 2.222, 3.333 };
    testPushBits(vec);
 
    SimpleRange<std::list<double>> sr = std::list<double>{ 1.111, 2.222, 3.333 };
    testPushBits(sr, sr.m_range.size());
}

TEST(PushBits, pushBitsValue)
{
    Blob b;
    pushBits(1.1, b);
    pushBits(5, b);
    pushBits('a', b);

    auto it = b.cbegin();
    BlobRange br = b;
    ASSERT_EQ(1.1, popBits<double>(br));
    ASSERT_EQ(5, popBits<int>(br));
    ASSERT_EQ('a', popBits<char>(br));
}

TEST(PushBits, popBitsThrowsOnOverflow)
{
    Blob b;
    pushBits(42, b); // push an int (sizeof(42) == 4

    double d {};
    ASSERT_THROW(popBits(d, b), std::exception);  // sizeof(d) == 8
}

TEST(PushBits, pushAndPopRanges)
{
    std::vector<double> vecOut(42, 3.14);
    std::vector<double> vecIn(42, 0);
    ASSERT_NE(vecOut, vecIn);

    Blob b;
    pushBits(vecOut, b);
    popBits(vecIn, b);
    ASSERT_EQ(vecOut, vecIn);
}