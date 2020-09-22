

#pragma once
#ifndef FILETABLE_H
#define FILETABLE_H

#include "Node.h"
#include "IntervalSequence.h"
#include <iterator>

namespace TxFs
{

class FileTable final
{
    uint16_t m_begin;
    uint16_t m_end;
    PageIndex m_next;
    uint8_t m_data[4084];

public:
    uint32_t m_checkSum;

public:
    FileTable() noexcept
        : m_begin(0)
        , m_end(sizeof(m_data))
        , m_next(PageIdx::INVALID)
    {
        // m_data[0] = 0;
        static_assert(sizeof(FileTable) == 4096);
    }

    constexpr void setNext(PageIndex next) noexcept { m_next = next; }
    constexpr PageIndex getNext() const noexcept { return m_next; }
    constexpr void clear() noexcept
    {
        m_begin = 0;
        m_end = sizeof(m_data);
    }

    void transferFrom(IntervalSequence& is) noexcept
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

    constexpr bool empty() const noexcept { return m_begin == 0 && m_end == sizeof(m_data); }

private:
    uint16_t* beginTable() const noexcept { return (uint16_t*) (m_data + m_end); }
    uint16_t* endTable() const noexcept { return (uint16_t*) (m_data + sizeof(m_data)); }
    PageIndex* beginIds() const noexcept { return (PageIndex*) m_data; }
    PageIndex* endIds() const noexcept { return (PageIndex*) (m_data + m_begin); }
    constexpr bool hasSpace(Interval iv) const noexcept
    {
        assert(iv.begin() < iv.end());
        return size_t(m_end - m_begin)
               >= ((iv.length()) > 1 ? 2U * sizeof(PageIndex) + sizeof(uint16_t) : sizeof(PageIndex));
    }
};

}

#endif // FILETABLE_H
