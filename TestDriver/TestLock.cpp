

#include "Test.h"
#include "../CompoundFs/Lock.h"

using namespace TxFs;

namespace
{
struct CheckRelease
{
    int m_calls = 0;
    std::string m_releaseOrder;

    void release() { m_calls++; }
    void sharedRelease() { m_releaseOrder += "S"; }
    void exclusiveRelease() { m_releaseOrder += "X"; }
};
}

TEST(Lock, compileTimeTests)
{
    CheckRelease cr;

    {
        Lock l(&cr, [](void* m) { static_cast<CheckRelease*>(m)->release(); });
        Lock l2 = std::move(l);
    }
    CHECK(cr.m_calls == 1);

    Lock l(&cr, [](void* m) { static_cast<CheckRelease*>(m)->release(); });
    l.release();
    CHECK(cr.m_calls == 2);
}

TEST(Lock, isSameMutex)
{
    CheckRelease cr;
    Lock l(&cr, [](void* m) { static_cast<CheckRelease*>(m)->release(); });

    CHECK(l.isSameMutex(&cr) == true);
    CheckRelease cr2;
    CHECK(l.isSameMutex(&cr2) == false);
}



TEST(CommitLock, sharedLockReleasesFirst)
{
    CheckRelease cr;
    auto xrel = [](void* m) { static_cast<CheckRelease*>(m)->exclusiveRelease(); };
    auto srel = [](void* m) { static_cast<CheckRelease*>(m)->sharedRelease(); };
    
    {
        CommitLock lock{Lock(&cr, xrel), Lock(&cr, srel)};
    }
    CHECK(cr.m_releaseOrder == "SX");

    CommitLock lock { Lock(&cr, xrel), Lock(&cr, srel) };
    auto xl = lock.release();
    CHECK(cr.m_releaseOrder == "SXS");

    xl.release();
    CHECK(cr.m_releaseOrder == "SXSX");

    CommitLock lock2 { Lock(&cr, xrel), Lock(&cr, srel) };
    lock2.release();
    CHECK(cr.m_releaseOrder == "SXSXSX");
}


