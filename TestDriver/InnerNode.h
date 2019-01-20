

#pragma once
#ifndef INNERNODE_H
#define INNERNODE_H

#include <algorithm>
#include <assert.h>

#include "Node.h"
#include "Blob.h"

namespace CompFs
{
#pragma pack(push)
#pragma pack(1)

class InnerNode : public Node
{
    uint8_t m_data[4087];
    Id m_leftMost;

public:
    InnerNode()
        : Node(0, sizeof(m_data), NodeType::Inner)
        , m_leftMost(INVALID_NODE)
    {
        m_data[0] = 0;
    }

    InnerNode(const Blob& key, Id left, Id right)
        : Node(0, sizeof(m_data), NodeType::Inner)
    {
        std::copy(key.begin(), key.end(), m_data + m_begin);
        m_leftMost = left;
        setPage(m_data + m_begin + key.size(), right);
        m_end -= sizeof(uint16_t);
        *beginTable() = m_begin;
        m_begin += key.size() + sizeof(Id);
    }

    size_t itemSize() const { return (sizeof(m_data) - m_end) / sizeof(uint16_t); }

    size_t bytesLeft() const
    {
        assert(m_end >= m_begin);
        return m_end - m_begin;
    }

    bool hasSpace(const Blob& key) const
    {
        size_t size = key.size() + sizeof(Id) + sizeof(uint16_t);
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

    Id getLeft(const uint16_t* it) const
    {
        if (it == beginTable())
            return m_leftMost;
        return getRight(it - 1);
    }

    Id getRight(const uint16_t* it) const
    {
        assert(it != endTable());
        return getPage(getKey(it).end());
    }

    void insert(const Blob& key, Id right)
    {
        assert(hasSpace(key));

        uint16_t begin = m_begin;
        std::copy(key.begin(), key.end(), m_data + m_begin);
        setPage(m_data + m_begin + key.size(), right);
        m_begin += key.size() + sizeof(Id);

        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);
        std::copy(beginTable(), it, beginTable() - 1);
        *(it - 1) = begin;
        m_end -= sizeof(uint16_t);
    }

    Id findPage(const Blob& key) const
    {
        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);
        if (it == endTable())
            return getLeft(it);
        Blob kentry(m_data + *it);
        if (kentry == key)
            return getRight(it);
        return getLeft(it);
    }

    void remove(const Blob& key)
    {
        assert(itemSize() > 0);

        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);

        // make sure that we can delete the key and its *right* PageId
        if (it == endTable())
            --it;
        else
        {
            if (Blob(m_data + *it) != key)
            {
                // must be the left PageId
                if (it == beginTable())
                    m_leftMost = getRight(it); // there is no element before
                else
                    --it;
            }
        }
        assert(getRight(it) == findPage(key));

        Blob kentry(m_data + *it);
        uint16_t index = *it;
        // copy what comes after to this place
        uint16_t size = kentry.size() + sizeof(Id);
        ;
        std::copy(m_data + *it + size, m_data + m_begin, kentry.begin());
        m_begin -= size;

        // copy what comes before to this place
        std::copy(beginTable(), it, beginTable() + 1);
        m_end += sizeof(uint16_t);

        // adjust indices of entries that came after
        for (it = beginTable(); it < endTable(); ++it)
            if (index < *it)
                *it -= size;
    }

    void split(InnerNode* rightNode, Blob& keyMiddle)
    {
        InnerNode tmp = *this;
        const uint16_t* it = tmp.findSplitPoint();
        assert(it != tmp.endTable());
        keyMiddle.assign(tmp.m_data + *it);
        fill(tmp, tmp.beginTable(), it);
        rightNode->fill(tmp, it + 1, tmp.endTable());
    }

    void split(InnerNode* rightNode, Blob& keyMiddle, const Blob& key, Id page)
    {
        split(rightNode, keyMiddle);
        KeyCmp keyCmp(m_data);
        if (keyCmp(*(endTable() - 1), key))
            rightNode->insert(key, page);
        else
            insert(key, page);
    }

private:
    Id getPage(const uint8_t* src) const
    {
        Id res;
        uint8_t* dest = (uint8_t*) &res;
        std::copy(src, src + sizeof(Id), dest);
        return res;
    }

    void setPage(uint8_t* dest, Id page)
    {
        const uint8_t* src = (uint8_t*) &page;
        std::copy(src, src + sizeof(Id), dest);
    }

    const uint16_t* findSplitPoint() const
    {
        size_t size = 0;
        for (uint16_t* it = beginTable(); it < endTable(); ++it)
        {
            Blob keyEntry(m_data + *it);
            size += keyEntry.size() + sizeof(Id);
            if (size > (m_begin / 2U))
                return it;
        }

        return endTable();
    }

    void fill(const InnerNode& node, const uint16_t* begin, const uint16_t* end)
    {
        m_begin = 0;
        m_end = uint16_t(sizeof(m_data) - sizeof(uint16_t) * (end - begin));
        uint16_t* destTable = beginTable();
        const uint8_t* data = node.m_data;
        for (const uint16_t* it = begin; it < end; ++it)
        {
            Blob key(data + *it);
            std::copy(key.begin(), key.end() + sizeof(Id), m_data + m_begin);
            *destTable = m_begin;
            destTable++;
            m_begin += key.size() + sizeof(Id);
        }

        m_leftMost = node.getLeft(begin);
    }
};

#pragma pack(pop)

}

#endif
