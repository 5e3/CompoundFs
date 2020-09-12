
#include <gtest/gtest.h>
#include "CompoundFs/TreeValue.h"

using namespace TxFs;

TEST(TreeValue, toAndFromString)
{
    ByteStringStream bss;
    TreeValue tv = "test";

    tv.toStream(bss);
    ByteStringView bsv = bss;
    ASSERT_EQ(bsv.size(), 5);
    TreeValue tv2 = TreeValue::fromStream(bsv);
}

TEST(TreeValue, wrongTypeIndexReturnsUnknownType)
{
    ByteStringStream bss;
    TreeValue tv = "test";

    tv.toStream(bss);
    ByteStringView bsv = bss;
    uint8_t* typeIndex = const_cast<uint8_t*>(bsv.data());
    *typeIndex = 100; // never do this!
    TreeValue tv2 = TreeValue::fromStream(bsv);
    ASSERT_EQ(tv2.getType(), TreeValue::Type::Unknown);
}

template <typename T>
struct TreeValueTester : ::testing::Test
{
    T initialize() const
    {
        auto values = std::make_tuple(
            FileDescriptor(1, 2, std::numeric_limits<uint64_t>::max()), 
            Folder { std::numeric_limits<uint32_t>::max() },
            Version { 1, 1, std::numeric_limits<uint32_t>::max() }, 
            std::numeric_limits<double>::max(),
            std::numeric_limits<uint64_t>::max(),
            std::string("test"));
        return std::get<T>(values);
    }

    TreeValueTester()
        : m_value(initialize())
    {}

    T m_value;


};

using TreeValueTypes = ::testing::Types<FileDescriptor, Folder, Version, double, uint64_t, std::string>;
TYPED_TEST_SUITE(TreeValueTester, TreeValueTypes);

TYPED_TEST(TreeValueTester, streamInOfStreamOutIsEqual)
{
    TreeValue tv = this->m_value;
    ByteStringStream bss;
    tv.toStream(bss);

    TreeValue tv2 = TreeValue::fromStream(bss);
    ASSERT_EQ(this->m_value, tv2.toValue<TypeParam>());
}

TYPED_TEST(TreeValueTester, toValueWithWrongTypeThrows)
{
    TreeValue tv;
    ASSERT_THROW(tv.toValue<TypeParam>(), std::exception);
}
