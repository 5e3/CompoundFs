

#pragma once
#ifndef FILETABLE_H
#define FILETABLE_H

#include "Node.h"
#include "IntervalSequence.h"
#include <iterator>

namespace TxFs
{

class FileTable : public Node
{
    uint8_t m_unused[3];
    PageIndex m_next;
    uint8_t m_data[4084];

public:
    FileTable()
        : Node(0, sizeof(m_data), NodeType::FileTable)
        , m_next(PageIdx::INVALID)
    {
        m_unused[0] = 0;
        m_data[0] = 0;
    }

    void setNext(PageIndex next) { m_next = next; }
    PageIndex getNext() const { return m_next; }
    void clear()
    {
        m_begin = 0;
        m_end = sizeof(m_data);
    }

    void transferFrom(IntervalSequence& is)
    {
        clear();
        while (!is.empty())
        {
            Interval iv = is.front();
            if (!hasSpace(iv))
                break;
            if (iv.length() > 1)
            {
                m_end -= sizeof(uint16_t);
                *beginTable() = m_begin / sizeof(PageIndex);
                *endIds() = iv.begin();
                m_begin += sizeof(PageIndex);
                *endIds() = iv.end();
                m_begin += sizeof(PageIndex);
            }
            else
            {
                *endIds() = iv.begin();
                m_begin += sizeof(PageIndex);
            }
            is.popFront();
        }
    }

    void insertInto(IntervalSequence& is) const
    {
        std::reverse_iterator<uint16_t*> it(endTable());
        std::reverse_iterator<uint16_t*> end(beginTable());
        for (size_t i = 0; i < m_begin / sizeof(PageIndex); ++i)
        {
            if (it != end && *it == i)
            {
                assert((i + 1) < m_begin / sizeof(PageIndex));
                Interval iv(*(beginIds() + i), *(beginIds() + i + 1));
                is.pushBack(iv);
                ++i;
                ++it;
            }
            else
            {
                Interval iv(*(beginIds() + i), *(beginIds() + i) + 1);
                is.pushBack(iv);
            }
        }
    }

    bool empty() const { return m_begin == 0 && m_end == sizeof(m_data); }

private:
    uint16_t* beginTable() const { return (uint16_t*) (m_data + m_end); }
    uint16_t* endTable() const { return (uint16_t*) (m_data + sizeof(m_data)); }
    PageIndex* beginIds() const { return (PageIndex*) m_data; }
    PageIndex* endIds() const { return (PageIndex*) (m_data + m_begin); }
    bool hasSpace(Interval iv) const
    {
        assert(iv.begin() < iv.end());
        return size_t(m_end - m_begin)
               >= ((iv.length()) > 1 ? 2U * sizeof(PageIndex) + sizeof(uint16_t) : sizeof(PageIndex));
    }
};

}

#endif // FILETABLE_H
