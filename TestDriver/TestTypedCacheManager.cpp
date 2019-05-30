

#include "stdafx.h"
#include "Test.h"
#include "SimpleFile.h"
#include "../CompoundFs/TypedCacheManager.h"
#include "../CompoundFs/FileTable.h"
#include "../CompoundFs/Leaf.h"

using namespace TxFs;

TEST(TypedCacheManager, newPageCallsCtor)
{
    SimpleFile sf;
    TypedCacheManager tcm(std::make_shared<CacheManager>(&sf));

    auto pdef = tcm.newPage<FileTable>();
    CHECK(pdef.m_page->getNext() == PageIdx::INVALID);
    CHECK(pdef.m_page->empty());
}

TEST(TypedCacheManager, makePageWritable)
{
    SimpleFile sf;
    auto cm = std::make_shared<CacheManager>(&sf);
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
    SimpleFile sf;
    TypedCacheManager tcm(std::make_shared<CacheManager>(&sf));

    {
        auto pdef = tcm.newPage<FileTable>();
        pdef.m_page->setNext(42);
        CHECK(pdef.m_index == 0);
    }

    auto pdef = tcm.repurpose<FileTable>(0);
    CHECK(pdef.m_page->getNext() == PageIdx::INVALID);
}
