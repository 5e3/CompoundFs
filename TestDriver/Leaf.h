

#pragma once
#ifndef LEAF_H
#define LEAF_H

#include <algorithm>
#include <assert.h>

#include "Node.h"
#include "Blob.h"

namespace TxFs
{
#pragma pack(push)
#pragma pack(1)

class Leaf : public Node
{
    uint8_t m_data[4083];
    Id m_prev;
    Id m_next;

public:
    typedef Node::Id Id;

public:
    Leaf(Id prev = INVALID_NODE, Id next = INVALID_NODE)
        : Node(0, sizeof(m_data), NodeType::Leaf)
        , m_prev(prev)
        , m_next(next)
    {
        m_data[0] = 0; // make the compiler happy
    }

    Id getNext() const { return m_next; }
    void setNext(Id next) { m_next = next; }

    size_t itemSize() const { return (sizeof(m_data) - m_end) / sizeof(uint16_t); }

    size_t bytesLeft() const
    {
        assert(m_end >= m_begin);
        return m_end - m_begin;
    }

    bool hasSpace(const Blob& key, const Blob& value) const
    {
        size_t size = sizeof(uint16_t) + key.size() + value.size();
        return size <= bytesLeft();
    }

    uint16_t* beginTable() const
    {
        assert(m_end <= sizeof(m_data));
        return (uint16_t*) &m_data[m_end];
    }

    uint16_t* endTable() const { return (uint16_t*) (m_data + sizeof(m_data)); }

    Blob getKey(const uint16_t* it) const
    {
        assert(it != endTable());
        return Blob(m_data + *it);
    }

    Blob getLowestKey() const
    {
        assert(beginTable() != endTable());
        return Blob(m_data + *beginTable());
    }

    Blob getValue(const uint16_t* it) const
    {
        assert(it != endTable());
        return Blob(getKey(it).end());
    }

    void insert(const Blob& key, const Blob& value)
    {
        assert(hasSpace(key, value));

        uint16_t begin = m_begin;
        std::copy(key.begin(), key.end(), &m_data[m_begin]);
        m_begin += key.size();
        std::copy(value.begin(), value.end(), &m_data[m_begin]);
        m_begin += value.size();

        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);
        std::copy(beginTable(), it, beginTable() - 1);
        *(it - 1) = begin;
        m_end -= sizeof(uint16_t);
    }

    uint16_t* find(const Blob& key) const
    {
        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);
        if (it == endTable())
            return it;
        Blob kentry(&m_data[*it]);
        if (kentry == key)
            return it;
        return endTable();
    }

    void remove(const Blob& key)
    {
        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);
        if (it == endTable())
            return;
        Blob kentry(&m_data[*it]);
        if (kentry != key)
            return;

        uint16_t index = *it;
        // copy what comes after to this place
        Blob ventry(kentry.end());
        uint16_t size = kentry.size() + ventry.size();
        std::copy(ventry.end(), &m_data[m_begin], kentry.begin());
        m_begin -= size;

        // copy what comes before to this place
        std::copy(beginTable(), it, beginTable() + 1);
        m_end += sizeof(uint16_t);

        // adjust indices of entries that came after
        for (it = beginTable(); it < endTable(); ++it)
            if (index < *it)
                *it -= size;
    }

    void split(Leaf* rightLeaf, const Blob& key, const Blob& value)
    {
        Leaf tmp = *this;
        const uint16_t* it = tmp.findSplitPoint();
        assert(it != tmp.endTable());
        fill(tmp, tmp.beginTable(), it);
        rightLeaf->fill(tmp, it, tmp.endTable());

        KeyCmp keyCmp(m_data);
        if (keyCmp(*(endTable() - 1), key))
            rightLeaf->insert(key, value);
        else
            insert(key, value);
    }

private:
    const uint16_t* findSplitPoint() const
    {
        size_t size = 0;
        for (uint16_t* it = beginTable(); it < endTable(); ++it)
        {
            Blob keyEntry(m_data + *it);
            size += keyEntry.size();
            Blob valueEntry(keyEntry.end());
            size += valueEntry.size();
            if (size > (m_begin / 2U))
                return it;
        }

        return endTable();
    }

    void fill(const Leaf& leaf, const uint16_t* begin, const uint16_t* end)
    {
        m_begin = 0;
        m_end = uint16_t(sizeof(m_data) - sizeof(uint16_t) * (end - begin));
        uint16_t* destTable = beginTable();
        uint8_t* data = (uint8_t*) leaf.m_data;
        for (const uint16_t* it = begin; it < end; ++it)
        {
            Blob key(data + *it);
            Blob value(key.end());
            std::copy(key.begin(), value.end(), m_data + m_begin);
            *destTable = m_begin;
            destTable++;
            m_begin += key.size() + value.size();
        }
    }
};

#pragma pack(pop)

}

#endif
