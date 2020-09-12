
#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include "CompoundFs/ByteString.h"

using namespace TxFs;

namespace 
{

void checkEqual(TxFs::ByteStringView bsv, const std::string& ref)
{
    if (bsv != ref)
        throw std::runtime_error("test failed");
}
}

TEST(ByteString, Ctor)
{
    ByteString a;
    ASSERT_EQ(a.size(), 0);

    ByteString b("Nils");
    ASSERT_EQ(b.size(), 4);

    ByteString c("Senta");
    ASSERT_NE(b, c);
    ASSERT_EQ(b, b);

    ByteString d = c;
    ASSERT_EQ(d, c);

    b = d;
    ASSERT_EQ(b, c);
}

TEST(ByteStringView, Transformations)
{
    ByteString bs("Senta");
    ByteStringView bsv = bs;
    ByteStringView bsv2 = bs;
    ASSERT_EQ(bsv.data(), bsv2.data());

    ByteString bs2(std::move(bs));
    bsv2 = bs2;
    ASSERT_EQ(bsv.data(), bsv2.data());

    bs = bsv; // reassignment after move is legal
    bsv = bs;
    ASSERT_NE(bsv.data(), bsv2.data());
    ASSERT_EQ(bs2, bsv);
}



TEST(ByteString, creation)
{
    ByteString b = "test";
    ASSERT_EQ(b, "test");

    b = "test2";
    ASSERT_EQ(b, "test2");

    ByteString b2 = std::move(b);
    ASSERT_EQ(b2, "test2");
    ASSERT_EQ(b.size(), 0);
}

TEST(ByteString, tooLongStringThrows)
{
    std::string s(256, '-');
    ASSERT_THROW(ByteString bs(s), std::exception);
    ASSERT_THROW(ByteStringView bsv(s), std::exception);
}

TEST(ByteStringView, fromStreamConvertsbackToStream)
{
    uint8_t stream[256];
    std::string s(255, '-');
    auto end = toStream(s, stream);
    ASSERT_EQ(end, stream + 256);

    ByteString bs = ByteStringView::fromStream(stream);
    ASSERT_EQ(bs, s);
    checkEqual(std::string(s), s);
}

TEST(ByteStringStream, canPushArithmetics)
{
    ByteStringStream bss;

    bss.push(int8_t(-1));
    ASSERT_EQ(static_cast<ByteStringView>(bss).size(), sizeof(int8_t));

    bss.push(3.14);
    ASSERT_EQ(static_cast<ByteStringView>(bss).size(), sizeof(int8_t) + sizeof(double));

    bss.push(true);
    ASSERT_EQ(static_cast<ByteStringView>(bss).size(), sizeof(int8_t) + sizeof(double) + sizeof(bool));
}

TEST(ByteStringStream, canPushStrings)
{
    ByteStringStream bss;

    bss.push(std::string("test"));
    ASSERT_EQ(bss, "test");
    bss.push(std::string("test"));
    ASSERT_EQ(bss, "testtest");
    bss.push('1');
    ASSERT_EQ(bss, "testtest1");
}

TEST(ByteStringStream, throwsIfStringIsTooBig)
{
    ByteStringStream bss;

    bss.push(std::string(255, '-'));
    ASSERT_THROW(bss.push(std::string(1,'X')), std::exception);
}

TEST(ByteStringStream, canPopArithmetics)
{
    ByteStringStream bss;
    auto values = std::make_tuple(3.14, true, int16_t(42));

    bss.push(std::get<0>(values));
    bss.push(std::get<1>(values));
    bss.push(std::get<2>(values));

    auto values2 = std::make_tuple(0.0, false, int16_t(0));
    ByteStringView bsv = bss;
    bsv = ByteStringStream::pop(std::get<0>(values2), bsv);
    bsv = ByteStringStream::pop(std::get<1>(values2), bsv);
    bsv = ByteStringStream::pop(std::get<2>(values2), bsv);

    ASSERT_EQ(values, values2);
}


