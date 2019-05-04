
#pragma once

#include "Node.h"
#include <assert.h>

namespace TxFs
{
class Interval
{
public:
    constexpr Interval() noexcept
        : m_begin(PageIdx::INVALID)
        , m_end(PageIdx::INVALID)
    {}

    constexpr Interval(PageIndex begin, PageIndex end) noexcept
        : m_begin(begin)
        , m_end(end)
    {
        assert(m_begin <= m_end);
    }

    constexpr explicit Interval(PageIndex begin) noexcept
        : m_begin(begin)
        , m_end(begin + 1)
    {
        assert(m_begin <= m_end);
    }

    constexpr bool operator==(Interval rhs) const noexcept { return m_begin == rhs.m_begin && m_end == rhs.m_end; }
    constexpr bool operator!=(Interval rhs) const noexcept { return !(operator==(rhs)); }
    constexpr bool operator<(Interval rhs) const noexcept
    {
        return m_begin == rhs.m_begin ? m_end < rhs.m_end : m_begin < rhs.m_begin;
    }

    constexpr uint32_t length() const noexcept { return m_end - m_begin; }
    constexpr bool empty() const noexcept { return length() == 0; }
    constexpr PageIndex begin() const noexcept { return m_begin; }
    constexpr PageIndex end() const noexcept { return m_end; }
    constexpr PageIndex& begin() noexcept { return m_begin; }
    constexpr PageIndex& end() noexcept { return m_end; }

private:
    PageIndex m_begin;
    PageIndex m_end;
};

}
