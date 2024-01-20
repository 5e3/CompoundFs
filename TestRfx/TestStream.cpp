

#include <gtest/gtest.h>
#include "Rfx/Stream.h"
#include <compare>

using namespace Rfx;

namespace StreamTest
{

struct VersionedType
{
    int m_i = 1;
    double m_d = 3.14;

    std::partial_ordering operator<=>(const VersionedType&) const noexcept = default;
};

template <typename TVisitor>
void forEachMember(VersionedType& value, TVisitor&& visitor)
{
    visitor(value.m_i);
    visitor(value.m_d);
}

struct EmptyType
{
    bool operator==(const EmptyType&) const { return true; }
};

template <typename TVisitor>
void forEachMember(EmptyType& value, TVisitor&& visitor)
{
}

}

using namespace StreamTest;



TEST(StreamOut, streamBitSimple)
{
    StreamOut out;
    out.write(1);

    std::vector<double> dvec = { 1.1, 2.2, 3.3 };
    out.write(dvec);
    auto blob = out.swapBlob();
    ASSERT_EQ(blob.size(), sizeof(int) + 1 + 1 + 3 * sizeof(double));
}

TEST(StreamOut, versionedType)
{
    VersionedType vt;
    StreamOut out;

    out.write(vt);
    auto blob = out.swapBlob();
    ASSERT_EQ(blob.size(), sizeof(int) + sizeof(double) + 1 + 1 + 1);
}

TEST(StreamIn, EmptyBlobThrows)
{
    Blob blob;
    ASSERT_THROW(StreamIn in(blob);, std::exception);
}

TEST(StreamIn, versionedType)
{
    VersionedType vt;
    StreamOut out;

    out.write(vt);
    auto blob = out.swapBlob();

    StreamIn in(blob);
    VersionedType vt2 { 1, 2.2 };
    ASSERT_NE(vt, vt2);
    in.read(vt2);
    ASSERT_EQ(vt, vt2);
}

template<typename T>
void testStreamOutStreamIn(const T& valOut)
{
    StreamOut out;
    out.write(valOut);
    auto blob = out.swapBlob();

    StreamIn in(blob);
    T valIn;
    in.read(valIn);
    ASSERT_EQ(valOut, valIn);
}

TEST(StreamInOut, vectorOfEmptyObjs)
{
    std::vector<EmptyType> vec(100, EmptyType {});
    testStreamOutStreamIn(vec);
}

TEST(StreamInOut, vectorOfVersionedObjs)
{
    std::vector<VersionedType> vec(100, VersionedType {});
    int i = 0;
    for (auto& e: vec)
        e.m_i = i++;

    testStreamOutStreamIn(vec);
}
TEST(StreamInOut, mapOfInts)
{
    std::map<int, int> m = { { 1, 1 }, { 2, 2 }};
    //int i = 0;
    //for (auto& e: vec)
    //    e.m_i = i++;

    testStreamOutStreamIn(m);
}
