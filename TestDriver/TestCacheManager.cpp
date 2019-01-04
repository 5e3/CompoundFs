
#include "stdafx.h"
#include "Test.h"
#include "CacheManager.h"


using namespace CompFs;

struct SimpleFile : RawFileInterface
{
    SimpleFile() : m_allocator(512) {}

    virtual Node::Id newPage()
    {
        Node::Id id = m_file.size();
        m_file.push_back(m_allocator.allocate());
        return id;
    }

    virtual void writePage(Node::Id id, std::shared_ptr<uint8_t> page)
    {
        auto p = m_file.at(id);
        std::copy(page.get(), page.get()+4096, p.get());
    }

    virtual void readPage(Node::Id id, std::shared_ptr<uint8_t> page) const
    {
        auto p = m_file.at(id);
        std::copy(p.get(), p.get()+4096, page.get());
    }

    std::vector<std::shared_ptr<uint8_t>> m_file;
    PageAllocator m_allocator;
};



TEST(CacheManager, newPageIsCachedButNotWritten)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    auto p = cm.newPage();
    {
        auto p2 = cm.getPage(p.second);
        CHECK(p.first == p2);
        *p2 = 0xaa;
    }
    p.first.reset();
    auto p2 = cm.getPage(p.second);
    CHECK(*p2 == 0xaa);
    CHECK (*sf.m_file.at(p.second) != *p2);
}

TEST(CacheManager, getPageIsCachedButNotWritten)
{
    SimpleFile sf;
    auto id = sf.newPage();
    *sf.m_file.at(id) = 42;

    CacheManager cm(&sf);
    auto p = cm.getPage(id);
    auto p2 = cm.getPage(id);
    CHECK(p == p2);

    *p = 99;
    CHECK(*sf.m_file.at(id) == 42);
}

TEST(CacheManager, trimReducesSizeOfCache)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for(int i=0; i<10; i++)
        cm.newPage();

    CHECK(cm.trim(20) == 10);
    CHECK(cm.trim(9) == 9);
    CHECK(cm.trim(5) == 5);
    CHECK(cm.trim(0) == 0);
}


TEST(CacheManager, newPageGetsWrittenToFileOnTrim)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for(int i=0; i<10; i++)
    {    
        auto p = cm.newPage().first;
        *p = i+1;
    }

    cm.trim(0);
    for(int i=0; i<10; i++)
        CHECK(*sf.m_file.at(i) == i+1);
}

TEST(CacheManager, pinnedPageDoNetGetWrittenToFileOnTrim)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for(int i=0; i<10; i++)
    {    
        auto p = cm.newPage().first;
        *p = i+1;
    }
    auto p1 = cm.getPage(0);
    auto p2 = cm.getPage(9);

    CHECK(cm.trim(0) == 2);

    for(int i=1; i<9; i++)
        CHECK(*sf.m_file.at(i) == i+1);
    
    CHECK(*sf.m_file.at(0) != *p1);
    CHECK(*sf.m_file.at(9) != *p2);
}

TEST(CacheManager, newPageGetsWrittenToFileOn2TrimOps)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for(int i=0; i<10; i++)
    {    
        auto p = cm.newPage().first;
        *p = i+1;
    }

    cm.trim(0);

    for(int i=0; i<10; i++)
    {    
        auto p = cm.getPage(i);
        *p = i+10;
        cm.pageDirty(i);
    }

    cm.trim(0);

    for(int i=0; i<10; i++)
        CHECK(*sf.m_file.at(i) == i+10);
}

TEST(CacheManager, newPageDontGetWrittenToFileOn2TrimOpsWithoutSettingDirty)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for(int i=0; i<10; i++)
    {    
        auto p = cm.newPage().first;
        *p = i+1;
    }

    cm.trim(0);

    for(int i=0; i<10; i++)
    {    
        auto p = cm.getPage(i);
        *p = i+10;  // change page but no pageDirty()
    }

    cm.trim(0);

    for(int i=0; i<10; i++)
        CHECK(*sf.m_file.at(i) == i+1);
}

TEST(CacheManager, dirtyPagesCanBeEvictedAndReadInAgain)
{
    SimpleFile sf;
    {
        CacheManager cm(&sf);
        for(int i=0; i<10; i++)
        {    
            auto p = cm.newPage().first;
            *p = i+1;
        }
        cm.trim(0);
    }

    CacheManager cm(&sf);
    for(int i=0; i<10; i++)
    {    
        auto p = cm.getPage(i);
        *p = i+10;
        cm.pageDirty(i);
    }
    cm.trim(0);

    for(int i=0; i<10; i++)
    {    
        auto p = cm.getPage(i);
        CHECK(*p == i+10);
    }
}


