#pragma once

#include "Node.h"
#include <memory>

namespace TxFs
{
/////////////////////////////////////////////////////////

template <typename TPage> struct ConstPageDef
{
    const std::shared_ptr<const TPage> m_page;
    const PageIndex m_index;

    ConstPageDef()
        : m_index(PageIdx::INVALID)
    {}

    ConstPageDef(std::shared_ptr<const TPage> page, PageIndex index)
        : m_page(page)
        , m_index(index)
    {}

    template <typename TPageDef> bool operator==(const TPageDef& rhs) const
    {
        return m_page == rhs.m_page && m_index == rhs.m_index;
    }
};

/////////////////////////////////////////////////////////

template <typename TPage> struct PageDef
{
    const std::shared_ptr<TPage> m_page;
    const PageIndex m_index;

    PageDef()
        : m_index(PageIdx::INVALID)
    {}

    PageDef(std::shared_ptr<TPage> page, PageIndex index)
        : m_page(page)
        , m_index(index)
    {}

    template <typename TPageDef> bool operator==(const TPageDef& rhs) const
    {
        return m_page == rhs.m_page && m_index == rhs.m_index;
    }
};
}
