

#pragma once
#ifndef INTERVALSEQUENCE_H
#define INTERVALSEQUENCE_H

#include "Node.h"
#include <deque>
#include <utility>
#include <algorithm>
#include <assert.h>


namespace CompFs
{
    class IntervalSequence
    {
    public:
        typedef std::pair<Node::Id, Node::Id> Interval;

    public:
        void pushBack(Interval iv)
        {
            assert(iv.first < iv.second);
            if (m_intervals.empty())
            {
                m_intervals.push_back(iv);
                return;
            }

            if (m_intervals.back().second == iv.first) // merge?
                m_intervals.back().second = iv.second;
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
            Interval iv = front();
            return iv.second - iv.first;
        }

        size_t totalLength() const
        {
            size_t len = 0;
            for (auto it = m_intervals.begin(); it != m_intervals.end(); ++it)
                len += it->second - it->first;
            return len;
        }

        Interval popFront()
        {
            assert(!empty());
            Interval iv = front();
            m_intervals.pop_front();
            return iv;
        }

        Interval popFront(uint32_t maxSize)
        {
            assert(!empty());
            Node::Id id = m_intervals.front().first;

            Node::Id size = std::min(maxSize, m_intervals.front().second - m_intervals.front().first);
            m_intervals.front().first += size;
            if (m_intervals.front().first == m_intervals.front().second)
                m_intervals.pop_front();

            return Interval(id, id + size);
        }

        bool empty() const
        {
            return m_intervals.empty();
        }

        void sort()
        {
            std::sort(m_intervals.begin(), m_intervals.end());
            if (m_intervals.size() < 2)
                return;
            size_t i = m_intervals.size() - 1;;
            size_t i2 = i--;
            for (; i2 != 0; --i)
            {
                if (m_intervals[i].second == m_intervals[i2].first)
                {
                    m_intervals[i].second = m_intervals[i2].second;
                    m_intervals.erase(m_intervals.begin()+i2);
                }
                i2 = i;
            }
        }

        size_t size() const { return m_intervals.size(); }

        bool operator==(const IntervalSequence& lhs) const { return m_intervals == lhs.m_intervals; }


    private:
        std::deque<Interval> m_intervals;
    };
}



#endif