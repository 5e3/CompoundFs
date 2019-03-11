

#pragma once

#include "CacheManager.h"
#include "PageDef.h"

namespace TxFs
{

class TypedCacheManager
{
public:
    TypedCacheManager(const std::shared_ptr<CacheManager>& cacheManager)
        : m_cacheManager(cacheManager)
    {}

    template <typename TPage, class... Args>
    PageDef<TPage> newPage(Args&&... args)
    {
        auto pdef = m_cacheManager->newPage();
        auto obj = new (pdef.m_page.get()) TPage(std::forward<Args>(args)...);
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

    template <typename TPage, class... Args>
    PageDef<TPage> repurpose(PageIndex index, bool forceNew, Args... args)
    {
        auto pdef = m_cacheManager->repurpose(index, forceNew);
        auto obj = new (pdef.m_page.get()) TPage(std::forward<Args>(args)...);
        return PageDef<TPage>(std::shared_ptr<TPage>(pdef.m_page, obj), pdef.m_index);
    }

    RawFileInterface* getRawFileInterface() const { return m_cacheManager->getRawFileInterface(); }
    Interval allocatePageInterval(size_t maxPages) { return m_cacheManager->allocatePageInterval(maxPages); }

private:
    std::shared_ptr<CacheManager> m_cacheManager;
};

}
