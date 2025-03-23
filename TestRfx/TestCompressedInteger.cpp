

#include <gtest/gtest.h>
#include "Rfx/CompressedInteger.h"
#include <limits>
#include <ranges>
#include <algorithm>

using namespace Rfx;

static_assert(compressedSize(0) == 1);
static_assert(compressedSize(127) == 1);
static_assert(compressedSize(128) == 2);
static_assert(compressedSize((1 << 14) - 1) == 2);
static_assert(compressedSize(1 << 14) == 3);
static_assert(compressedSize(0xffffffff) == 5);
static_assert(compressedSize(std::numeric_limits<size_t>::max()) == 10);

using CompInt = CompressedInteger<size_t>;
using SRule = StreamRule<CompInt>;


namespace
{
struct SimpleStreamOut
{
    std::vector<std::byte> m_vector;

    void write(std::byte value) { m_vector.push_back(value); }
};

struct SimpleStreamIn
{
    std::vector<std::byte>::const_iterator m_pos;

    void read(std::byte& value)
    {
        value = *m_pos;
        ++m_pos;
    }
};

void testReadWrite(size_t val)
{
    CompInt ci { val };
    SimpleStreamOut out;
    SRule::write(ci, out);
    ASSERT_EQ(out.m_vector.size(), compressedSize(ci.m_value));

    SimpleStreamIn in { out.m_vector.cbegin() };
    CompInt ci2;
    SRule::read(ci2, in);

    ASSERT_EQ(ci.m_value, ci2.m_value);
}

}

TEST(CompressedInteger, readWrite)
{
    testReadWrite(0);
    testReadWrite(127);
    testReadWrite(128);
    testReadWrite((1 << 14) - 1);
    testReadWrite(1 << 14);
    testReadWrite(0xffff);
    testReadWrite(0xffffff);
    testReadWrite(12345678);
}

TEST(CompressedInteger, readThrowsOnIllegalInput)
{
    // create vec with 10x 0xff - thats definitly not a legal compressed integer
    std::vector<std::byte> vec(compressedSize(std::numeric_limits<size_t>::max()));
    std::ranges::fill (vec, std::byte { 0xff });

    SimpleStreamIn in { vec.cbegin() };
    CompInt ci;
    ASSERT_THROW(SRule::read(ci, in), std::exception);
}

TEST(CompressedInteger, canHandleMaxInt)
{
    testReadWrite(std::numeric_limits<size_t>::max());
}