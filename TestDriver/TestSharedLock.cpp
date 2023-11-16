
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
    std::thread t1([dsl2=dsl]() mutable { dsl2.lock(); }); // nb: made a copy
    dsl.waitForWaiting(1);
    ASSERT_EQ(dsl.getWaiting(), 1);
    dsl.unlock_shared();
    dsl.unlock_shared();
    t1.join();
}

TEST(DebugSharedLock, blockedUnlock)
{
    DebugSharedLock dsl;
    dsl.lock_shared();
    int done = 0;
    std::thread t1([&]() {
        dsl.lock();
        done = 1;
        dsl.unlock();
        done = 2;
    });

    dsl.waitForWaiting(1); // wait until writer locks
    dsl.makeUnlockBlock(); // next unlock() blocks
    dsl.unlock_shared();   // release writer
    dsl.waitForWaiting(0); // wait until writer entered the lock
    dsl.lock_shared();     // wait until writer calls unlock
    ASSERT_EQ(done, 1);
    dsl.unlock_shared();   
    dsl.unlockRelease();   // release writer from unlock
    t1.join();
    ASSERT_EQ(done, 2);
}
