

#include <gtest/gtest.h>
#include "CompoundFs/LogPage.h"

using namespace TxFs;

TEST(LogPage, size)
{
    LogPage log(0);
    ASSERT_EQ(sizeof(log) , 4096);
}

TEST(LogPage, checkSignature)
{
    LogPage log(100);
    ASSERT_TRUE(log.checkSignature(100));
    ASSERT_TRUE(!log.checkSignature(1));
}

TEST(LogPage, storingRetrieving)
{
    LogPage lp(100);

    auto pageCopies = lp.getPageCopies();
    for (uint32_t i = 0; i < 500; i++)
        pageCopies.push_back({ i, i });

    for (auto pc: pageCopies)
        ASSERT_TRUE(lp.pushBack(pc));

    ASSERT_EQ(lp.getPageCopies() , pageCopies);
}

TEST(LogPage, sizeTests)
{
    LogPage lp(100);

    for (uint32_t i = 0; i < LogPage::MAX_ENTRIES; i++)
        ASSERT_TRUE(lp.pushBack({ i, i }));

    ASSERT_EQ(lp.size() , LogPage::MAX_ENTRIES);
    ASSERT_TRUE(!lp.pushBack({ 1, 1 }));
}

TEST(LogPage, pushBackIterator)
{
    LogPage lp(100);

    for (uint32_t i = 0; i < LogPage::MAX_ENTRIES / 2; i++)
        ASSERT_TRUE(lp.pushBack({ i, i }));

    auto pageCopies = lp.getPageCopies();
    ASSERT_EQ(lp.pushBack(pageCopies.begin(), pageCopies.end()) , pageCopies.end());
    ASSERT_EQ(lp.size() , 2 * (LogPage::MAX_ENTRIES / 2) );
    ASSERT_TRUE(lp.pushBack({ 1, 1 }));
    ASSERT_TRUE(!lp.pushBack({ 1, 1 }));
}

TEST(LogPage, beginEndCanBeUsedToAppend)
{
    LogPage lp(100);

    for (uint32_t i = 0; i < LogPage::MAX_ENTRIES; i++)
        ASSERT_TRUE(lp.pushBack({ i, i }));

    auto pageCopies = lp.getPageCopies();
    pageCopies.insert(pageCopies.end(), lp.begin(), lp.end());

    ASSERT_EQ(pageCopies.size() , 2 * LogPage::MAX_ENTRIES);
}
