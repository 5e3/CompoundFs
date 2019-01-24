
#pragma once

#include "Node.h"
#include <assert.h>

namespace TxFs
{
class Interval
{
public:
    Interval()
        : m_begin(0)
        , m_end(0)
    {}

    Interval(PageIndex begin, PageIndex end)
        : m_begin(begin)
        , m_end(end)
    {
        assert(m_begin <= m_end);
    }

    explicit Interval(PageIndex begin)
        : m_begin(begin)
        , m_end(begin + 1)
    {}

    bool operator==(Interval rhs) const { return m_begin == rhs.m_begin && m_end == rhs.m_end; }
    bool operator!=(Interval rhs) const { return !(operator==(rhs)); }
    bool operator<(Interval rhs) const { return m_begin == rhs.m_begin ? m_end < rhs.m_end : m_begin < rhs.m_begin; }

    uint32_t length() const { return m_end - m_begin; }
    bool empty() const { return length() == 0; }
    PageIndex begin() const { return m_begin; }
    PageIndex end() const { return m_end; }
    PageIndex& begin() { return m_begin; }
    PageIndex& end() { return m_end; }

private:
    PageIndex m_begin;
    PageIndex m_end;
};

}
