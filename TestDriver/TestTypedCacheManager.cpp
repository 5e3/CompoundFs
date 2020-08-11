

#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/TypedCacheManager.h"
#include "../CompoundFs/FileTable.h"
#include "../CompoundFs/Leaf.h"

using namespace TxFs;

TEST(TypedCacheManager, newPageCallsCtor)
{
    MemoryFile memFile;
    TypedCacheManager tcm(std::make_shared<CacheManager>(&memFile));

    auto pdef = tcm.newPage<FileTable>();
    CHECK(pdef.m_page->getNext() == PageIdx::INVALID);
    CHECK(pdef.m_page->empty());
}

TEST(TypedCacheManager, makePageWritable)
{
    MemoryFile memFile;
    auto cm = std::make_shared<CacheManager>(&memFile);
    TypedCacheManager tcm(cm);

    {
        auto pdef = tcm.newPage<FileTable>();
        pdef.m_page->setNext(42);
        CHECK(pdef.m_index == 0);
    }

    {
        auto pdef = tcm.loadPage<FileTable>(0);
        CHECK(pdef.m_page->getNext() == 42);
        auto pdef2 = tcm.makePageWritable(pdef);
        pdef2.m_page->setNext(1010);
    }
    cm->trim(0);

    auto pdef = tcm.loadPage<FileTable>(0);
    CHECK(pdef.m_page->getNext() == 1010);
}

TEST(TypedCacheManager, repurposeCallsCtor)
{
    MemoryFile memFile;
    TypedCacheManager tcm(std::make_shared<CacheManager>(&memFile));

    {
        auto pdef = tcm.newPage<FileTable>();
        pdef.m_page->setNext(42);
        CHECK(pdef.m_index == 0);
    }

    auto pdef = tcm.repurpose<FileTable>(0);
    CHECK(pdef.m_page->getNext() == PageIdx::INVALID);
}
