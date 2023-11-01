#pragma once

#include "Node.h"
#include <memory>

namespace TxFs
{
/////////////////////////////////////////////////////////
template <typename TPage>
struct PageDef;

template <typename TPage>
struct ConstPageDef final
{
    std::shared_ptr<const TPage> m_page;
    PageIndex m_index;

    constexpr ConstPageDef() noexcept
        : m_index(PageIdx::INVALID)
    {
    }

    constexpr ConstPageDef(const PageDef<TPage>& pdef) noexcept
        : m_page(pdef.m_page)
        , m_index(pdef.m_index)
    {
    }

    constexpr ConstPageDef(std::shared_ptr<const TPage> page, PageIndex index) noexcept
        : m_page(page)
        , m_index(index)
    {
    }

    constexpr bool operator==(const ConstPageDef& rhs) const noexcept
    {
        return m_page == rhs.m_page && m_index == rhs.m_index;
    }
};

/////////////////////////////////////////////////////////

template <typename TPage>
struct PageDef final
{
    std::shared_ptr<TPage> m_page;
    PageIndex m_index;

    constexpr PageDef() noexcept
        : m_index(PageIdx::INVALID)
    {
    }

    constexpr PageDef(std::shared_ptr<TPage> page, PageIndex index) noexcept
        : m_page(page)
        , m_index(index)
    {
    }

    constexpr bool operator==(const PageDef& rhs) const noexcept
    {
        return m_page == rhs.m_page && m_index == rhs.m_index;
    }
};

/////////////////////////////////////////////////////////

template <typename TTo, typename TFrom>
ConstPageDef<TTo> staticPageDefCast(const ConstPageDef<TFrom>& from)
{
    return ConstPageDef<TTo>(std::static_pointer_cast<const TTo>(from.m_page), from.m_index);
}

template <typename TTo, typename TFrom>
ConstPageDef<TTo> staticPageDefCast(ConstPageDef<TFrom>&& from)
{
    PageIndex idx = PageIdx::INVALID;
    std::swap(idx, from.m_index);
    return ConstPageDef<TTo>(std::static_pointer_cast<const TTo>(std::move(from.m_page)), idx);
}

template <typename TTo, typename TFrom>
PageDef<TTo> staticPageDefCast(const PageDef<TFrom>& from)
{
    return ConstPageDef<TTo>(std::static_pointer_cast<TTo>(from.m_page), from.m_index);
}

template <typename TTo, typename TFrom>
PageDef<TTo> staticPageDefCast(PageDef<TFrom>&& from)
{
    PageIndex idx = PageIdx::INVALID;
    std::swap(idx, from.m_index);
    return ConstPageDef<TTo>(std::static_pointer_cast<TTo>(std::move(from.m_page)), idx);
}

/////////////////////////////////////////////////////////

}