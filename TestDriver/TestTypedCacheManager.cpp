


#include <gtest/gtest.h>
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/TypedCacheManager.h"
#include "../CompoundFs/FileTable.h"
#include "../CompoundFs/Leaf.h"

using namespace TxFs;

TEST(TypedCacheManager, newPageCallsCtor)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    auto pdef = tcm.newPage<FileTable>();
    ASSERT_EQ(pdef.m_page->getNext() , PageIdx::INVALID);
    ASSERT_TRUE(pdef.m_page->empty());
}

TEST(TypedCacheManager, makePageWritable)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);

    {
        auto pdef = tcm.newPage<FileTable>();
        pdef.m_page->setNext(42);
        ASSERT_EQ(pdef.m_index , 0);
    }

    {
        auto pdef = tcm.loadPage<FileTable>(0);
        ASSERT_EQ(pdef.m_page->getNext() , 42);
        auto pdef2 = tcm.makePageWritable(pdef);
        pdef2.m_page->setNext(1010);
    }
    cm->trim(0);

    auto pdef = tcm.loadPage<FileTable>(0);
    ASSERT_EQ(pdef.m_page->getNext() , 1010);
}

TEST(TypedCacheManager, repurposeCallsCtor)
{
    auto cm = std::make_shared<CacheManager>(std::make_unique<MemoryFile>());
    TypedCacheManager tcm(cm);
    {
        auto pdef = tcm.newPage<FileTable>();
        pdef.m_page->setNext(42);
        ASSERT_EQ(pdef.m_index , 0);
    }

    auto pdef = tcm.repurpose<FileTable>(0);
    ASSERT_EQ(pdef.m_page->getNext() , PageIdx::INVALID);
}
