#pragma once

#include "Node.h"
#include <memory>

namespace TxFs
{
/////////////////////////////////////////////////////////
template <typename TPage> struct PageDef;

template <typename TPage> struct ConstPageDef
{
    std::shared_ptr<const TPage> m_page;
    PageIndex m_index;

    ConstPageDef()
        : m_index(PageIdx::INVALID)
    {}

    ConstPageDef(const PageDef<TPage>& pdef)
        : m_page(pdef.m_page)
        , m_index(pdef.m_index)
    {}

    ConstPageDef(std::shared_ptr<const TPage> page, PageIndex index)
        : m_page(page)
        , m_index(index)
    {}

    bool operator==(const ConstPageDef& rhs) const { return m_page == rhs.m_page && m_index == rhs.m_index; }
};

/////////////////////////////////////////////////////////

template <typename TPage> struct PageDef
{
    std::shared_ptr<TPage> m_page;
    PageIndex m_index;

    PageDef()
        : m_index(PageIdx::INVALID)
    {}

    PageDef(std::shared_ptr<TPage> page, PageIndex index)
        : m_page(page)
        , m_index(index)
    {}

    bool operator==(const PageDef& rhs) const { return m_page == rhs.m_page && m_index == rhs.m_index; }
};
}
