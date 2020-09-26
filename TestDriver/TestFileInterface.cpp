

#include <gtest/gtest.h>

#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/FileIo.h"

using namespace TxFs;


TEST(FileIo, modifiedSignedPagesGetDetected)
{
    uint8_t page[4096];
    MemoryFile mf;
    mf.newInterval(1);
    writeSignedPage(&mf, 0, page);
    readSignedPage(&mf, 0, page);
    page[0]++;
    mf.writePage(0, 0, page, page + sizeof(page));
    ASSERT_FALSE(testReadSignedPage(&mf, 0, page));
    ASSERT_THROW(readSignedPage(&mf, 0, page), std::exception);
}














