

#pragma once
#ifndef INTERVALSEQUENCE_H
#define INTERVALSEQUENCE_H

#include "Node.h"
#include "Interval.h"
#include <deque>
#include <utility>
#include <algorithm>
#include <assert.h>

namespace TxFs
{

class IntervalSequence final
{
public:
    void pushBack(Interval iv)
    {
        assert(!iv.empty());
        assert(iv.begin() != PageIdx::INVALID);
        m_totalLength += iv.length();
        if (m_intervals.empty())
        {
            m_intervals.push_back(iv);
            return;
        }

        if (m_intervals.back().end() == iv.begin()) // merge?
            m_intervals.back().end() = iv.end();
        else
            m_intervals.push_back(iv);
    }

    Interval back() const
    {
        assert(!empty());
        return m_intervals.back();
    }

    Interval front() const
    {
        assert(!empty());
        return m_intervals.front();
    }

    size_t frontLength() const
    {
        assert(!empty());
        return front().length();
    }

    size_t totalLength() const
    {
        return m_totalLength;
    }

    Interval popFront()
    {
        assert(!empty());
        Interval iv = front();
        m_intervals.pop_front();
        m_totalLength -= iv.length();
        return iv;
    }

    Interval popFront(uint32_t maxSize)
    {
        assert(!empty());
        PageIndex id = m_intervals.front().begin();

        PageIndex size = std::min(maxSize, m_intervals.front().length());
        m_intervals.front().begin() += size;
        if (m_intervals.front().empty())
            m_intervals.pop_front();

        m_totalLength -= size;
        return Interval(id, id + size);
    }

    void clear() 
    { 
        m_intervals.clear(); 
        m_totalLength = 0;
    }

    bool empty() const { return m_intervals.empty(); }

    void sort()
    {
        std::sort(m_intervals.begin(), m_intervals.end());
        if (m_intervals.size() < 2)
            return;
        size_t i = m_intervals.size() - 1;
        ;
        size_t i2 = i--;
        for (; i2 != 0; --i)
        {
            if (m_intervals[i].end() == m_intervals[i2].begin())
            {
                m_intervals[i].end() = m_intervals[i2].end();
                m_intervals.erase(m_intervals.begin() + i2);
            }
            i2 = i;
        }
    }

    void moveTo(IntervalSequence& is)
    {
        for (auto iv: m_intervals)
            is.pushBack(iv);
        clear();
    }

    size_t size() const { return m_intervals.size(); }

    bool operator==(const IntervalSequence& lhs) const { return m_intervals == lhs.m_intervals; }

    std::deque<Interval>::const_iterator begin() const { return m_intervals.begin(); }
    std::deque<Interval>::const_iterator end() const { return m_intervals.end(); }

private:
    std::deque<Interval> m_intervals;
    size_t m_totalLength = 0;
};
}

#endif
