
#pragma once

#include "Node.h"
#include <assert.h>

namespace CompFs
{
    class Interval
    {
    public:
      Interval()
        : m_begin(0)
        , m_end(0)
      {
      }

      Interval(Node::Id begin, Node::Id end)
        : m_begin(begin)
        , m_end(end)
      {
        assert(m_begin <= m_end);
      }

      //Interval(std::pair<Node::Id, Node::Id> p)
      //  : m_begin(p.first)
      //  , m_end(p.second)
      //{
      //  assert(m_begin <= m_end);
      //}

      bool operator==(Interval rhs) const { return m_begin == rhs.m_begin && m_end == rhs.m_end; }
      bool operator!=(Interval rhs) const { return !(operator==(rhs)); }
      bool operator<(Interval rhs) const { return m_begin == rhs.m_begin ? m_end < rhs.m_end : m_begin < rhs.m_begin; }


      uint32_t length() const { return m_end - m_begin; }
      bool empty() const { return length() == 0; }
      Node::Id begin() const { return m_begin; }
      Node::Id end() const { return m_end; }
      Node::Id& begin() { return m_begin; }
      Node::Id& end() { return m_end; }

    private:
      Node::Id m_begin;
      Node::Id m_end;
    };

}