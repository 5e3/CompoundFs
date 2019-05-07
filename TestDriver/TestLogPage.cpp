
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
