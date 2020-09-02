

#include <gtest/gtest.h>
#include "../CompoundFs/LockProtocol.h"

#include <mutex>
#include <shared_mutex>

using namespace TxFs;

using SimpleLockProtocoll = LockProtocol<std::shared_mutex, std::mutex>;

TEST(LockProtocol, writeAccessIsExclusiveLock)
{
    SimpleLockProtocoll slp;
    auto lock = slp.writeAccess();

    ASSERT_TRUE(!slp.tryWriteAccess());

    lock.release();
    auto l2 = slp.tryWriteAccess();
    ASSERT_TRUE(l2);
    ASSERT_TRUE(!slp.tryWriteAccess());
}

TEST(LockProtocol, readAccessIsSharedLock)
{
    SimpleLockProtocoll slp;
    auto lock = slp.readAccess();

    ASSERT_TRUE(slp.tryReadAccess());
}

TEST(LockProtocol, commitAccessThrowsOnWrongParam)
{
    SimpleLockProtocoll slp;
    ASSERT_THROW(slp.commitAccess(slp.readAccess()), std::exception);

    SimpleLockProtocoll slp2;
    SimpleLockProtocoll slp3;
    ASSERT_THROW(slp2.commitAccess(slp3.writeAccess()), std::exception);
}

TEST(LockProtocol, canCommitAccessTwice)
{
    SimpleLockProtocoll slp;
    auto lock = slp.writeAccess();
    auto commit = slp.commitAccess(std::move(lock));
    lock = commit.release();
    auto commit2 = slp.commitAccess(std::move(lock));
    lock = commit2.release();
}

TEST(LockProtocol, readAccessIsNotBlockedByWriteAccess)
{
    SimpleLockProtocoll slp;
    auto wlock = slp.writeAccess();

    ASSERT_TRUE(slp.tryReadAccess());
}

TEST(LockProtocol, readAccessIsBlockedByCommitAccess)
{
    SimpleLockProtocoll slp;
    auto commitLock = slp.commitAccess(slp.writeAccess());

    ASSERT_TRUE(!slp.tryReadAccess());
}

TEST(LockProtocol, commitAccessIsBlockedByReadAccess)
{
    SimpleLockProtocoll slp;
    auto rlock = slp.readAccess();
    auto commitLock = slp.tryCommitAccess(slp.writeAccess());

    ASSERT_EQ(std::get_if<CommitLock>(&commitLock) , nullptr);
}

//TEST(LockProtocol, readAccessCannotStarveCommitAccess)
//{
//    SimpleLockProtocoll slp;
//    auto rlock = slp.readAccess();
//
//    std::thread t([slp = &slp]() { slp->commitAccess(slp->writeAccess()); });
//
//    // busy wait
//    while (true)
//    {
//        std::this_thread::yield();
//        auto wlock = slp.tryWriteAccess();
//        if (!wlock)
//            break;
//    }
//    ASSERT_TRUE(!slp.tryReadAccess());
//    rlock.release();
//    //rlock = slp.readAccess();
//    t.join();
//}
