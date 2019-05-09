
#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/LogPage.h"

using namespace TxFs;

TEST(LogPage, size)
{
    LogPage log(0);
    CHECK(sizeof(log) == 4096);
}

TEST(LogPage, checkSignature)
{
    LogPage log(100);
    CHECK(log.checkSignature(100));
    CHECK(!log.checkSignature(1));
}


TEST(LogPage, storingRetrieving)
{
    LogPage lp(100);

    auto pageCopies = lp.getPageCopies();
    for (uint32_t i = 0; i < 500; i++)
        pageCopies.push_back({ i,i });

    for (auto pc : pageCopies)
        CHECK(lp.pushBack(pc));

   CHECK(lp.getPageCopies() == pageCopies);
}
