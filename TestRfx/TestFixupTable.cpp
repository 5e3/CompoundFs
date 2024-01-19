
#include <gtest/gtest.h>
#include "Rfx/FixupTable.h"
#include <algorithm>

using namespace Rfx;

TEST(FixupTable, readEmpty)
{
    std::vector<std::byte> bits { std::byte { 0 } };
    FixupTable ft;
    auto it = ft.read(bits.begin(), bits.end());
    ASSERT_EQ(bits.end(), it);
    ASSERT_EQ(ft.begin(), ft.end());
}

TEST(FixupTable, completelyEmptyBitsThrow)
{
    std::vector<std::byte> bits;
    FixupTable ft;

    ASSERT_THROW(ft.read(bits.begin(), bits.end()), std::exception);
}

TEST(FixupTable, illegalCompressedBitsThrow)
{
    std::vector<std::byte> bits(20, std::byte{0xff});
    FixupTable ft;

    ASSERT_THROW(ft.read(bits.begin(), bits.end()), std::exception);
}

TEST(FixupTable, create)
{
    FixupTable ft;
    std::vector<std::function<void(size_t)>> fixups;

    for (size_t i = 0; i < 100; i++)
        fixups.push_back(ft.nextFixup(100));

    size_t size = 1000;
    for (auto f: fixups)
        f(size++);

    size = 1000;
    for (auto f: ft)
        ASSERT_EQ(f, size++ - 100);
}

TEST(FixupTable, writeRead)
{
    FixupTable ft;
    std::vector<std::function<void(size_t)>> fixups;

    for (size_t i = 0; i < 200; i++)
        fixups.push_back(ft.nextFixup(0));

    size_t size = 100;
    for (auto f: fixups)
        f(size++);

    std::vector<std::byte> bits(ft.sizeInBytes());
    auto it = ft.write(bits.begin());
    ASSERT_EQ(bits.end(), it);

    FixupTable ft2;
    ft2.read(bits.begin(), bits.end());
    ASSERT_TRUE(std::ranges::equal(ft, ft2));
    ASSERT_EQ(ft, ft2);
    ASSERT_EQ(ft.sizeInBytes(), bits.size());
}



 







