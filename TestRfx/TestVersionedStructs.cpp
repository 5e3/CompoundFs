

#include <gtest/gtest.h>
#include "Rfx/StreamRule.h"
#include "Rfx/Stream.h"
#include "VersionedStructs.h"
#include "RandomGen.h"

using namespace VersionedStructs;
using namespace Rfx;

static consteval auto toTernary(int val)
{
    std::array<int,3> res;
    std::get<2>(res) = val % 3;
    val /= 3;
    std::get<1>(res) = val % 3;
    val /= 3;
    std::get<0>(res) = val % 3;
    return res;
}


template<int Version>
static auto createVersionedStruct()
{
    constexpr auto tp = toTernary(Version);
  
    C<tp[0]> c { B<tp[1]> { A<tp[2]> {} } };
    return c;
}



TEST(VersionedStructs, canCallForEachMember)
{
    auto c = createVersionedStruct<3>();
    RandomGen gen;
    forEachMember(c, gen);
    StreamOut out;
    out.write(c);

    auto blob = out.swapBlob();
    auto c2 = createVersionedStruct<1>();

    StreamIn in(blob);
    in.read(c2);
    ASSERT_TRUE(c == c2);
    ASSERT_EQ(c, c2);
}
