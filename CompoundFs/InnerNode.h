

#pragma once
#ifndef INNERNODE_H
#define INNERNODE_H

#include <algorithm>
#include <assert.h>

#include "Node.h"
#include "Blob.h"

namespace TxFs
{
#pragma pack(push)
#pragma pack(1)

class InnerNode : public Node
{
    uint8_t m_data[4087];
    PageIndex m_leftMost;

public:
    InnerNode() noexcept
        : Node(0, sizeof(m_data), NodeType::Inner)
        , m_leftMost(PageIdx::INVALID)
    {
        m_data[0] = 0;
    }

    InnerNode(const Blob& key, PageIndex left, PageIndex right) noexcept
        : Node(0, sizeof(m_data), NodeType::Inner)
    {
        std::copy(key.begin(), key.end(), m_data + m_begin);
        m_leftMost = left;
        setPageId(m_data + m_begin + key.size(), right);
        m_end -= sizeof(uint16_t);
        *beginTable() = m_begin;
        m_begin += key.size() + sizeof(PageIndex);
    }

    constexpr bool empty() const noexcept { return m_begin == 0; }
    constexpr size_t itemSize() const noexcept { return (sizeof(m_data) - m_end) / sizeof(uint16_t); }

    constexpr size_t bytesLeft() const noexcept
    {
        assert(m_end >= m_begin);
        return m_end - m_begin;
    }

    constexpr bool hasSpace(const Blob& key) const noexcept
    {
        size_t size = key.size() + sizeof(PageIndex) + sizeof(uint16_t);
        return size <= bytesLeft();
    }

    constexpr uint16_t* beginTable() const noexcept
    {
        assert(m_end <= sizeof(m_data));
        return (uint16_t*) &m_data[m_end];
    }

    constexpr uint16_t* endTable() const noexcept { return (uint16_t*) (m_data + sizeof(m_data)); }

    constexpr BlobRef getKey(const uint16_t* it) const noexcept
    {
        assert(it != endTable());
        return BlobRef(m_data + *it);
    }

    constexpr PageIndex getLeft(const uint16_t* it) const noexcept
    {
        if (it == beginTable())
            return m_leftMost;
        return getRight(it - 1);
    }

    PageIndex getRight(const uint16_t* it) const noexcept
    {
        assert(it != endTable());
        return getPageId(getKey(it).end());
    }

    void insert(const Blob& key, PageIndex right) noexcept
    {
        assert(hasSpace(key));

        uint16_t begin = m_begin;
        std::copy(key.begin(), key.end(), m_data + m_begin);
        setPageId(m_data + m_begin + key.size(), right);
        m_begin += key.size() + sizeof(PageIndex);

        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);
        std::copy(beginTable(), it, beginTable() - 1);
        *(it - 1) = begin;
        m_end -= sizeof(uint16_t);
    }

    PageIndex findPage(const Blob& key) const noexcept
    {
        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);
        if (it == endTable())
            return getLeft(it);
        BlobRef kentry(m_data + *it);
        if (kentry == key)
            return getRight(it);
        return getLeft(it);
    }

    void remove(const Blob& key) noexcept
    {
        assert(itemSize() > 0);

        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);

        // make sure that we can delete the key and its *right* PageId
        if (it == endTable())
            --it;
        else
        {
            if (BlobRef(m_data + *it) != key)
            {
                // must be the left PageId
                if (it == beginTable())
                    m_leftMost = getRight(it); // there is no element before
                else
                    --it;
            }
        }
        assert(getRight(it) == findPage(key));

        BlobRef kentry(m_data + *it);
        uint16_t index = *it;
        // copy what comes after to this place
        uint16_t size = kentry.size() + sizeof(PageIndex);
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

    // returns middle key
    Blob split(InnerNode* rightNode) noexcept
    {
        const InnerNode tmp = *this;
        const uint16_t* const it = tmp.findSplitPoint();
        assert(it != tmp.endTable());
        fill(tmp, tmp.beginTable(), it);
        rightNode->fill(tmp, it + 1, tmp.endTable());
        Blob middleKey(tmp.m_data + *it);
        return middleKey;
    }

    // returns middle key
    Blob split(InnerNode* rightNode, const Blob& key, PageIndex page) noexcept
    {
        auto keyMiddle = split(rightNode);
        KeyCmp keyCmp(m_data);
        if (keyCmp(*(endTable() - 1), key))
            rightNode->insert(key, page);
        else
            insert(key, page);
        return keyMiddle;
    }

private:
    PageIndex getPageId(const uint8_t* src) const noexcept
    {
        PageIndex res;
        uint8_t* dest = (uint8_t*) &res;
        std::copy(src, src + sizeof(PageIndex), dest);
        return res;
    }

    void setPageId(uint8_t* dest, PageIndex page)
    {
        const uint8_t* src = (uint8_t*) &page;
        std::copy(src, src + sizeof(PageIndex), dest);
    }

    const uint16_t* findSplitPoint() const noexcept
    {
        size_t size = 0;
        for (uint16_t* it = beginTable(); it < endTable(); ++it)
        {
            BlobRef keyEntry(m_data + *it);
            size += keyEntry.size() + sizeof(PageIndex);
            if (size > (m_begin / 2U))
                return it;
        }

        return endTable();
    }

    void fill(const InnerNode& node, const uint16_t* begin, const uint16_t* end) noexcept
    {
        m_begin = 0;
        m_end = uint16_t(sizeof(m_data) - sizeof(uint16_t) * (end - begin));
        uint16_t* destTable = beginTable();
        const uint8_t* data = node.m_data;
        for (const uint16_t* it = begin; it < end; ++it)
        {
            BlobRef key(data + *it);
            std::copy(key.begin(), key.end() + sizeof(PageIndex), m_data + m_begin);
            *destTable = m_begin;
            destTable++;
            m_begin += key.size() + sizeof(PageIndex);
        }

        m_leftMost = node.getLeft(begin);
    }
};

#pragma pack(pop)

}

#endif
