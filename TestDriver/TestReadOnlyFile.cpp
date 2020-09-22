



#include <gtest/gtest.h>
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/ReadOnlyFile.h"
#include "CompoundFs/CacheManager.h"
#include "CompoundFs/CommitHandler.h"
#include "CompoundFs/FileIo.h"


using namespace TxFs;

TEST(ReadOnlyFile, UnmodifiedCacheManagerCanCommitReadOnly)
{
    MemoryFile mf;
    mf.newInterval(1);
    uint8_t page[4096];
    writeSignedPage(&mf, 0, page);
    CacheManager cm(std::make_unique<ReadOnlyFile<MemoryFile>>(std::move(mf)));
    cm.loadPage(0);
    cm.getCommitHandler().commit();

}
