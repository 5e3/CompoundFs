

#pragma once
#ifndef LEAF_H
#define LEAF_H

#include <algorithm>
#include <assert.h>

#include "Node.h"
#include "ByteString.h"
#include "TableKeyCompare.h"

namespace TxFs
{
#pragma pack(push)
#pragma pack(1)

class Leaf final : public Node
{
    uint8_t m_data[4083];
    PageIndex m_prev;
    PageIndex m_next;

public:
    Leaf(PageIndex prev = PageIdx::INVALID, PageIndex next = PageIdx::INVALID) noexcept
        : Node(0, sizeof(m_data), NodeType::Leaf)
        , m_prev(prev)
        , m_next(next)
    {
        m_data[0] = 0; // make the compiler happy
    }

    constexpr PageIndex getNext() const noexcept { return m_next; }
    constexpr void setNext(PageIndex next) noexcept { m_next = next; }
    constexpr PageIndex getPrev() const noexcept { return m_prev; }
    constexpr void setPrev(PageIndex prev) noexcept { m_prev = prev; }
    constexpr bool empty() const noexcept { return m_begin == 0; }

    constexpr size_t nofItems() const noexcept { return (sizeof(m_data) - m_end) / sizeof(uint16_t); }

    constexpr size_t bytesLeft() const noexcept
    {
        assert(m_end >= m_begin);
        return m_end - m_begin;
    }

    constexpr bool hasSpace(ByteStringView key, ByteStringView value) const noexcept
    {
        size_t size = sizeof(uint16_t) + key.size() + value.size() + 2;
        return size <= bytesLeft();
    }

    uint16_t toIndex(const uint8_t* pos) const { return static_cast<uint16_t>(pos - m_data);}

    uint16_t* beginTable() const noexcept
    {
        assert(m_end <= sizeof(m_data));
        return (uint16_t*) &m_data[m_end];
    }

    uint16_t* endTable() const noexcept { return (uint16_t*) (m_data + sizeof(m_data)); }

    ByteStringView getKey(const uint16_t* it) const noexcept
    {
        assert(it != endTable());
        return ByteStringView::fromStream(m_data + *it);
    }

    ByteStringView getLowestKey() const noexcept
    {
        assert(beginTable() != endTable());
        return ByteStringView::fromStream(m_data + *beginTable());
    }

    ByteStringView getValue(const uint16_t* it) const noexcept
    {
        assert(it != endTable());
        auto key = getKey(it);
        return ByteStringView::fromStream(key.data() + key.size());
    }

    void insert(ByteStringView key, ByteStringView value) noexcept
    {
        assert(hasSpace(key, value));

        uint16_t begin = m_begin;
        auto end = toStream(key, &m_data[m_begin]);
        end = toStream(value, end);
        //m_begin += key.size() + 1 + value.size() + 1;
        m_begin = toIndex(end);

        uint16_t* it = lowerBound(key);
        std::copy(beginTable(), it, beginTable() - 1);
        *(it - 1) = begin;
        m_end -= sizeof(uint16_t);
    }

    uint16_t* lowerBound(ByteStringView key) const noexcept
    {
        TableKeyCompare keyCmp(m_data);
        return std::lower_bound(beginTable(), endTable(), key, keyCmp);
    }

    uint16_t* find(ByteStringView key) const noexcept
    {
        uint16_t* it = lowerBound(key);
        if (it == endTable())
            return it;
        if (getKey(it) == key)
            return it;
        return endTable();
    }

    void remove(ByteStringView key) noexcept
    {
        uint16_t* it = lowerBound(key);
        if (it == endTable())
            return;
        auto kentry = getKey(it);
        if (kentry != key)
            return;

        uint16_t index = *it;
        // copy what comes after to this place
        auto ventry = getValue(it);    
        uint16_t size = toIndex(ventry.end()) - *it;
        std::copy(ventry.end(), (const uint8_t*)&m_data[m_begin], &m_data[*it]);
        m_begin -= size;

        // copy what comes before to this place
        std::copy(beginTable(), it, beginTable() + 1);
        m_end += sizeof(uint16_t);

        // adjust indices of entries that came after
        for (it = beginTable(); it < endTable(); ++it)
            if (index < *it)
                *it -= size;
    }

    void split(Leaf* rightLeaf, ByteStringView key, ByteStringView value) noexcept
    {
        Leaf tmp = *this;
        const uint16_t* it = tmp.findSplitPoint();
        assert(it != tmp.endTable());
        fill(tmp, tmp.beginTable(), it);
        rightLeaf->fill(tmp, it, tmp.endTable());

        TableKeyCompare keyCmp(m_data);
        if (keyCmp(*(endTable() - 1), key))
            rightLeaf->insert(key, value);
        else
            insert(key, value);
    }

private:
    const uint16_t* findSplitPoint() const noexcept
    {
        size_t size = 0;
        for (uint16_t* it = beginTable(); it < endTable(); ++it)
        {
            size += toIndex(getValue(it).end()) - *it;
            if (size > (m_begin / 2U))
                return it;
        }

        return endTable();
    }

    void fill(const Leaf& leaf, const uint16_t* begin, const uint16_t* end) noexcept
    {
        m_begin = 0;
        m_end = uint16_t(sizeof(m_data) - sizeof(uint16_t) * (end - begin));
        uint16_t* destTable = beginTable();
        for (const uint16_t* it = begin; it < end; ++it)
        {
            auto end = toStream(leaf.getKey(it), &m_data[m_begin]);
            end = toStream(leaf.getValue(it), end);
            *destTable = m_begin;
            destTable++;
            m_begin = toIndex(end);
        }
    }
};

#pragma pack(pop)

}

#endif
