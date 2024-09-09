


#include <gtest/gtest.h>
#include <ranges>
#include "RandomGen.h"
#include "Rfx/StreamRule.h"

using namespace Rfx;

TEST(RandomGen, createsInts)
{
    RandomGen in;
    int i=0;
    in.read(i);
    ASSERT_NE(i, 0);
}
TEST(RandomGen, createsDoubles)
{
    RandomGen in;
    double d = 0;
    in.read(d);
    ASSERT_NE(d, 0);
}
TEST(RandomGen, createsStrings)
{
    RandomGen in;
    std::string s;
    in.read(s);
    if (!s.empty()) 
        ASSERT_EQ(s[0], s.size() +'A');
}
TEST(RandomGen, createsVectorOfStrings)
{
    RandomGen in;
    std::vector<std::string> vec;
    in.read(vec);
    if (!vec.empty())
        if (!vec[0].empty())
            ASSERT_EQ(vec[0][0], vec[0].size() + 'A');
}