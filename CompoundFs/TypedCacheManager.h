

#pragma once

#include "CacheManager.h"
#include "PageDef.h"

namespace TxFs
{

class TypedCacheManager final
{
public:
    TypedCacheManager(const std::shared_ptr<CacheManager>& cacheManager)
        : m_cacheManager(cacheManager)
    {}

    template <typename TPage, class... Ts>
    PageDef<TPage> newPage(Ts&&... args)
    {
        static_assert(sizeof(TPage::m_checkSum) == sizeof(uint32_t)); // must have m_checkSum
        auto pdef = m_cacheManager->newPage();
        auto obj = new (pdef.m_page.get()) TPage(std::forward<Ts>(args)...);
        return PageDef<TPage>(std::shared_ptr<TPage>(pdef.m_page, obj), pdef.m_index);
    }

    template <typename TPage>
    ConstPageDef<TPage> loadPage(PageIndex index)
    {
        auto page = m_cacheManager->loadPage(index);
        return ConstPageDef<TPage>(std::shared_ptr<TPage>(page.m_page, (TPage*) page.m_page.get()), page.m_index);
    }

    template <typename TPage>
    PageDef<TPage> makePageWritable(const ConstPageDef<TPage>& loadedPage)
    {
        m_cacheManager->setPageDirty(loadedPage.m_index);
        return PageDef<TPage>(std::const_pointer_cast<TPage>(loadedPage.m_page), loadedPage.m_index);
    }

    template <typename TPage, class... Ts>
    PageDef<TPage> repurpose(PageIndex index, Ts... args)
    {
        auto pdef = m_cacheManager->repurpose(index);
        auto obj = new (pdef.m_page.get()) TPage(std::forward<Ts>(args)...);
        return PageDef<TPage>(std::shared_ptr<TPage>(pdef.m_page, obj), pdef.m_index);
    }

    FileInterface* getFileInterface() const { return m_cacheManager->getFileInterface(); }
    Interval allocatePageInterval(size_t maxPages) { return m_cacheManager->allocatePageInterval(maxPages); }

private:
    std::shared_ptr<CacheManager> m_cacheManager;
};

}
