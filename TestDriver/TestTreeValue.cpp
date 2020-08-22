
#include <gtest/gtest.h>
#include "../CompoundFs/TreeValue.h"

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

TEST(TreeValue, comparison)
{
    TreeValue tv = 1.1;
    //ASSERT_EQ(tv, 1.1);
}
