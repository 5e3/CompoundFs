

#pragma once
#ifndef INNERNODE_H
#define INNERNODE_H

#include <algorithm>
#include <assert.h>

#include "Node.h"
#include "ByteString.h"

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

    InnerNode(const ByteString& key, PageIndex left, PageIndex right) noexcept
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
    constexpr size_t nofItems() const noexcept { return (sizeof(m_data) - m_end) / sizeof(uint16_t); }
    constexpr size_t size() const noexcept { return sizeof(m_data) - bytesLeft(); }

    constexpr size_t bytesLeft() const noexcept
    {
        assert(m_end >= m_begin);
        return m_end - m_begin;
    }

    constexpr bool hasSpace(const ByteString& key) const noexcept
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

    constexpr ByteStringView getKey(const uint16_t* it) const noexcept
    {
        assert(it != endTable());
        return ByteStringView(m_data + *it);
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

    void insert(const ByteString& key, PageIndex right) noexcept
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

    uint16_t* findKey(ByteStringView key) const noexcept
    {
        KeyCmp keyCmp(m_data);
        auto it = std::lower_bound(beginTable(), endTable(), key, keyCmp);
        assert(nofItems() > 0);
        if (it == endTable())
            --it;
        return it;
    }

    PageIndex findPage(const ByteString& key) const noexcept
    {
        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);
        if (it == endTable())
            return getLeft(it);
        ByteStringView kentry(m_data + *it);
        if (kentry == key)
            return getRight(it);
        return getLeft(it);
    }

    void remove(const ByteString& key) noexcept
    {
        assert(nofItems() > 0);

        KeyCmp keyCmp(m_data);
        uint16_t* it = std::lower_bound(beginTable(), endTable(), key, keyCmp);

        // make sure that we can delete the key and its *right* PageId
        if (it == endTable())
            --it;
        else
        {
            if (ByteStringView(m_data + *it) != key)
            {
                // must be the left PageId
                if (it == beginTable())
                    m_leftMost = getRight(it); // there is no element before
                else
                    --it;
            }
        }
        assert(getRight(it) == findPage(key));

        ByteStringView kentry(m_data + *it);
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
    ByteString split(InnerNode* rightNode) noexcept
    {
        const InnerNode tmp = *this;
        const uint16_t* const it = tmp.findSplitPoint(m_begin / 2U);
        assert(it != tmp.endTable());
        fill(tmp, tmp.beginTable(), it);
        rightNode->fill(tmp, it + 1, tmp.endTable());
        ByteString middleKey(tmp.m_data + *it);
        return middleKey;
    }

    // returns middle key
    ByteString split(InnerNode* rightNode, const ByteString& key, PageIndex page) noexcept
    {
        auto keyMiddle = split(rightNode);

        if (key < keyMiddle)
            insert(key, page);
        else
            rightNode->insert(key, page);

        return keyMiddle;
    }

    static ByteString redistribute(InnerNode& left, InnerNode& right, ByteStringView parentKey) noexcept
    {
        if (left.size() < right.size())
        {
            const InnerNode tmp = right;
            size_t splitPoint = (right.m_begin - left.m_begin) / 2;
            const uint16_t* const sp = tmp.findSplitPoint(splitPoint);
            assert(sp != tmp.endTable());
            left.insert(parentKey, right.m_leftMost);
            left.copyToBack(tmp, tmp.beginTable(), sp);
            right.reset();
            right.copyToFront(tmp, sp + 1, tmp.endTable());
            return tmp.getKey(sp);
        }
        else
        {
            const InnerNode tmp = left;
            size_t splitPoint = left.m_begin - size_t(left.m_begin - right.m_begin) / 2;
            const uint16_t* const sp = tmp.findSplitPoint(splitPoint);
            assert(sp != tmp.endTable());
            left.reset();
            left.copyToFront(tmp, tmp.beginTable(), sp);
            right.insert(parentKey, right.m_leftMost);
            right.copyToFront(tmp, sp + 1, tmp.endTable());
            return tmp.getKey(sp);
        }
    }

    constexpr void reset() noexcept
    {
        m_begin = 0;
        m_end = sizeof(m_data);
        m_leftMost = PageIdx::INVALID;
    }

    void copyToFront(const InnerNode& from, const uint16_t* begin, const uint16_t* end) noexcept
    {
        size_t idxSize = end - begin;
        auto idx = beginTable() - idxSize;
        auto data = m_data + m_begin;
        for (auto it = begin; it < end; ++it)
        {
            *idx = static_cast<uint16_t>(data - m_data);
            ByteStringView key(from.m_data + *it);
            data = std::copy(key.begin(), key.end() + sizeof(PageIndex), data);
            idx++;
        }
        assert(idx == beginTable());
        m_begin = static_cast<uint16_t>(data - m_data);
        m_end -= static_cast<uint16_t>(idxSize * sizeof(uint16_t));
        assert(m_begin <= m_end);
        m_leftMost = from.getLeft(begin);
    }

    void copyToBack(const InnerNode& from, const uint16_t* begin, const uint16_t* end) noexcept
    {
        size_t idxSize = end - begin;
        auto idx = std::copy(beginTable(), endTable(), beginTable() - idxSize);
        auto data = m_data + m_begin;
        for (auto it = begin; it < end; ++it)
        {
            *idx = static_cast<uint16_t>(data - m_data);
            ByteStringView key(from.m_data + *it);
            data = std::copy(key.begin(), key.end() + sizeof(PageIndex), data);
            idx++;
        }
        assert(idx == endTable());
        m_begin = static_cast<uint16_t>(data - m_data);
        m_end -= static_cast<uint16_t>(idxSize * sizeof(uint16_t));
        assert(m_begin <= m_end);
    }

    bool canMergeWith(const InnerNode& right, ByteStringView parentKey) noexcept
    {
        return bytesLeft() >= (right.size() + parentKey.size() + sizeof(PageIndex) + sizeof(uint16_t));
    }

    void mergeWith(const InnerNode& right, ByteStringView parentKey) noexcept
    {
        insert(parentKey, right.m_leftMost);
        copyToBack(right, right.beginTable(), right.endTable());
    }

private:
    PageIndex getPageId(const uint8_t* src) const noexcept
    {
        PageIndex res;
        uint8_t* dest = (uint8_t*) &res;
        std::copy(src, src + sizeof(PageIndex), dest);
        return res;
    }

    void setPageId(uint8_t* dest, PageIndex page) noexcept
    {
        const uint8_t* src = (uint8_t*) &page;
        std::copy(src, src + sizeof(PageIndex), dest);
    }

    const uint16_t* findSplitPoint(size_t splitPoint) const noexcept
    {
        size_t size = 0;
        for (uint16_t* it = beginTable(); it < endTable(); ++it)
        {
            ByteStringView keyEntry(m_data + *it);
            size += keyEntry.size() + sizeof(PageIndex);
            if (size > splitPoint)
                return it;
        }

        return endTable();
    }

    void fill(const InnerNode& node, const uint16_t* begin, const uint16_t* end) noexcept
    {
        reset();
        copyToFront(node, begin, end);
    }
};

#pragma pack(pop)

}

#endif
