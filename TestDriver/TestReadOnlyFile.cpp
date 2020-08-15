


#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/ReadOnlyFile.h"
#include "../CompoundFs/CacheManager.h"
#include "../CompoundFs/CommitHandler.h"


using namespace TxFs;

TEST(ReadOnlyFile, UnmodifiedCacheManagerCanCommitReadOnly)
{
    MemoryFile mf;
    mf.newInterval(1);
    CacheManager cm(std::make_unique<ReadOnlyFile<MemoryFile>>(std::move(mf)));
    cm.loadPage(0);
    cm.buildCommitHandler().commit();

}
