
#include <gtest/gtest.h>
#include <thread>

#include "CompoundFs/SharedLock.h"

using namespace TxFs;

TEST(DebugSharedLock, nolock)
{
    DebugSharedLock dsl;
    dsl.lock_shared();
    dsl.lock_shared();
    ASSERT_EQ(dsl.getWaiting(), 0);
    dsl.waitForWaiting(0);
}

TEST(DebugSharedLock, lock)
{
    DebugSharedLock dsl;
    dsl.lock_shared();
    dsl.lock_shared();
    std::thread t1([dslp = &dsl]() { dslp->lock(); });
    dsl.waitForWaiting(1);
    ASSERT_EQ(dsl.getWaiting(), 1);
    dsl.unlock_shared();
    dsl.unlock_shared();
    t1.join();
}