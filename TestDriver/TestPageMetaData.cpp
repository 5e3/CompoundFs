

#include <gtest/gtest.h>
#include "../CompoundFs/PageMetaData.h"

#include <algorithm>
#include <random>


using namespace TxFs;

TEST(PrioritizedPage, sortOrder)
{
    std::vector<PrioritizedPage> psis;
    psis.emplace_back(PageClass::Dirty, 5, 0, 0);
    psis.emplace_back(PageClass::Dirty, 3, 5, 1);
    psis.emplace_back(PageClass::Dirty, 3, 4, 2);
    psis.emplace_back(PageClass::New, 0, 5, 3);
    psis.emplace_back(PageClass::New, 0, 4, 4);
    psis.emplace_back(PageClass::New, 0, 0, 5);
    psis.emplace_back(PageClass::Read, 3, 1, 6);
    psis.emplace_back(PageClass::Read, 3, 0, 7);
    psis.emplace_back(PageClass::Read, 2, 10, 8);

    auto psis2 = psis;
    std::shuffle(psis2.begin(), psis2.end(), std::mt19937(std::random_device()()));
    std::sort(psis2.begin(), psis2.end());
    ASSERT_EQ(psis2 , psis);
}
