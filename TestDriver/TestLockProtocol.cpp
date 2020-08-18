

#include "Test.h"
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/LockProtocol.h"

#include <mutex>
#include <shared_mutex>

using namespace TxFs;

using SimpleLockProtocoll = LockProtocol<std::shared_mutex, std::mutex>;

TEST(LockProtocol, writeAccessIsExclusiveLock)
{
    SimpleLockProtocoll slp;
    auto lock = slp.writeAccess();

    CHECK(!slp.tryWriteAccess());

    lock.release();
    auto l2 = slp.tryWriteAccess();
    CHECK(l2);
    CHECK(!slp.tryWriteAccess());
}

TEST(LockProtocol, readAccessIsSharedLock)
{
    SimpleLockProtocoll slp;
    auto lock = slp.readAccess();

    CHECK(slp.tryReadAccess());
}

TEST(LockProtocol, commitAccessThrowsOnWrongParam)
{
    try
    {
        SimpleLockProtocoll slp;
        auto clock = slp.commitAccess(slp.readAccess());
        CHECK(false);
    }
    catch (std::exception&) {}
    try
    {
        SimpleLockProtocoll slp;
        SimpleLockProtocoll slp2;
        auto clock = slp.commitAccess(slp2.writeAccess());
        CHECK(false);
    }
    catch (std::exception&) {}
}

TEST(LockProtocol, readAccessIsNotBlockedByWriteAccess)
{
    SimpleLockProtocoll slp;
    auto wlock = slp.writeAccess();

    CHECK(slp.tryReadAccess());
}

TEST(LockProtocol, readAccessIsBlockedByCommitAccess)
{
    SimpleLockProtocoll slp;
    auto commitLock = slp.commitAccess(slp.writeAccess());

    CHECK(!slp.tryReadAccess());
}

TEST(LockProtocol, commitAccessIsBlockedByReadAccess)
{
    SimpleLockProtocoll slp;
    auto rlock = slp.readAccess();
    auto commitLock = slp.tryCommitAccess(slp.writeAccess());

    CHECK(std::get_if<CommitLock>(&commitLock) == nullptr);
}

TEST(LockProtocol, readAccessCannotStarveCommitAccess)
{
    SimpleLockProtocoll slp;
    auto rlock = slp.readAccess();

    std::thread t([slp = &slp]() { slp->commitAccess(slp->writeAccess()); });

    // busy wait
    while (true)
    {
        std::this_thread::yield();
        auto wlock = slp.tryWriteAccess();
        if (!wlock)
            break;
    }
    CHECK(!slp.tryReadAccess());
    rlock.release();
    //rlock = slp.readAccess();
    t.join();
}