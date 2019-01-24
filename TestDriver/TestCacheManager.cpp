
#include "stdafx.h"
#include "Test.h"
#include "../CompoundFs/CacheManager.h"
#include <algorithm>

using namespace TxFs;

struct SimpleFile : RawFileInterface
{
    SimpleFile()
        : m_allocator(512)
    {}

    virtual PageIndex newPage()
    {
        PageIndex id = (PageIndex) m_file.size();
        m_file.push_back(m_allocator.allocate());
        return id;
    }

    virtual void writePage(PageIndex id, std::shared_ptr<uint8_t> page)
    {
        auto p = m_file.at(id);
        std::copy(page.get(), page.get() + 4096, p.get());
    }

    virtual void readPage(PageIndex id, std::shared_ptr<uint8_t> page) const
    {
        auto p = m_file.at(id);
        std::copy(p.get(), p.get() + 4096, page.get());
    }

    std::vector<std::shared_ptr<uint8_t>> m_file;
    PageAllocator m_allocator;
};

///////////////////////////////////////////////////////////////////////////////

TEST(CacheManager, newPageIsCachedButNotWritten)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    PageIndex idx;
    {
        auto p = cm.newPage();
        idx = p.m_index;
        {
            auto p2 = cm.loadPage(p.m_index);
            CHECK(p == p2);
            *p.m_page = 0xaa;
        }
    }
    auto p2 = cm.loadPage(idx);
    CHECK(*p2.m_page == 0xaa);
    CHECK(*sf.m_file.at(idx) != *p2.m_page);
}

TEST(CacheManager, loadPageIsCachedButNotWritten)
{
    SimpleFile sf;
    auto id = sf.newPage();
    *sf.m_file.at(id) = 42;

    CacheManager cm(&sf);
    auto p = cm.loadPage(id);
    auto p2 = cm.loadPage(id);
    CHECK(p == p2);

    //*p = 99; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    CHECK(*sf.m_file.at(id) == 42);
}

TEST(CacheManager, trimReducesSizeOfCache)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for (int i = 0; i < 10; i++)
        auto page = cm.newPage();

    CHECK(cm.trim(20) == 10);
    CHECK(cm.trim(9) == 9);
    CHECK(cm.trim(5) == 5);
    CHECK(cm.trim(0) == 0);
}

TEST(CacheManager, newPageGetsWrittenToFileOnTrim)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }

    cm.trim(0);
    for (int i = 0; i < 10; i++)
        CHECK(*sf.m_file.at(i) == i + 1);
}

TEST(CacheManager, pinnedPageDoNotGetWrittenToFileOnTrim)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }
    auto p1 = cm.loadPage(0);
    auto p2 = cm.loadPage(9);

    CHECK(cm.trim(0) == 2);

    for (int i = 1; i < 9; i++)
        CHECK(*sf.m_file.at(i) == i + 1);

    CHECK(*sf.m_file.at(0) != *p1.m_page);
    CHECK(*sf.m_file.at(9) != *p2.m_page);
}

TEST(CacheManager, newPageGetsWrittenToFileOn2TrimOps)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }

    cm.trim(0);

    for (int i = 0; i < 10; i++)
    {
        auto p = std::const_pointer_cast<uint8_t>(cm.loadPage(i).m_page); // don't do that! !!!!!!!!!!!!!!!
        *p = i + 10;
        cm.setPageDirty(i);
    }

    cm.trim(0);

    for (int i = 0; i < 10; i++)
        CHECK(*sf.m_file.at(i) == i + 10);
}

TEST(CacheManager, newPageDontGetWrittenToFileOn2TrimOpsWithoutSettingDirty)
{
    SimpleFile sf;
    CacheManager cm(&sf);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.newPage().m_page;
        *p = i + 1;
    }

    cm.trim(0);

    for (int i = 0; i < 10; i++)
    {
        auto p = std::const_pointer_cast<uint8_t>(cm.loadPage(i).m_page); // don't do that! !!!!!!!!!!!!!!!
        *p = i + 10;                                                      // change page but no pageDirty()
    }

    cm.trim(0);

    for (int i = 0; i < 10; i++)
        CHECK(*sf.m_file.at(i) == i + 1);
}

TEST(CacheManager, dirtyPagesCanBeEvictedAndReadInAgain)
{
    SimpleFile sf;
    {
        CacheManager cm(&sf);
        for (int i = 0; i < 10; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i + 1;
        }
        cm.trim(0);
    }

    CacheManager cm(&sf);
    for (int i = 0; i < 10; i++)
    {
        auto p = std::const_pointer_cast<uint8_t>(cm.loadPage(i).m_page);
        *p = i + 10;
        cm.setPageDirty(i);
    }
    cm.trim(0);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.loadPage(i).m_page;
        CHECK(*p == i + 10);
    }
}

TEST(CacheManager, dirtyPagesCanBeEvictedTwiceAndReadInAgain)
{
    SimpleFile sf;
    {
        CacheManager cm(&sf);
        for (int i = 0; i < 10; i++)
        {
            auto p = cm.newPage().m_page;
            *p = i + 1;
        }
        cm.trim(0);
    }

    CacheManager cm(&sf);
    for (int i = 0; i < 10; i++)
    {
        auto p = std::const_pointer_cast<uint8_t>(cm.loadPage(i).m_page);
        *p = i + 10;
        cm.setPageDirty(i);
    }
    cm.trim(0);

    for (int i = 0; i < 10; i++)
    {
        auto p = std::const_pointer_cast<uint8_t>(cm.loadPage(i).m_page);
        *p = i + 20;
        cm.setPageDirty(i);
    }
    cm.trim(0);
    CHECK(sf.m_file.size() == 20);

    for (int i = 0; i < 10; i++)
    {
        auto p = cm.loadPage(i).m_page;
        CHECK(*p == i + 20);
    }
}

TEST(PageSortItem, sortOrder)
{
    std::vector<CacheManager::PageSortItem> psis;
    psis.emplace_back(CacheManager::PageSortItem::DirtyRead, 5, 0, 0);
    psis.emplace_back(CacheManager::PageSortItem::DirtyRead, 3, 5, 1);
    psis.emplace_back(CacheManager::PageSortItem::DirtyRead, 3, 4, 2);
    psis.emplace_back(CacheManager::PageSortItem::New, 0, 5, 3);
    psis.emplace_back(CacheManager::PageSortItem::New, 0, 4, 4);
    psis.emplace_back(CacheManager::PageSortItem::New, 0, 0, 5);
    psis.emplace_back(CacheManager::PageSortItem::Read, 3, 1, 6);
    psis.emplace_back(CacheManager::PageSortItem::Read, 3, 0, 7);
    psis.emplace_back(CacheManager::PageSortItem::Read, 2, 10, 8);

    auto psis2 = psis;
    std::random_shuffle(psis2.begin(), psis2.end());
    std::sort(psis2.begin(), psis2.end());
    CHECK(psis2 == psis);
}
