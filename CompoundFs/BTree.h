

#pragma once
#ifndef BTREE_H
#define BTREE_H

#include "PageDef.h"
#include "TypedCacheManager.h"
#include "Blob.h"

#include <vector>
#include <memory>
#include <optional>
#include <variant>

namespace TxFs
{
class Leaf;
class InnerNode;
class BTree;

//////////////////////////////////////////////////////////////////////////

class Cursor
{
    friend class BTree;

public:
    constexpr Cursor() noexcept = default;
    Cursor(const std::shared_ptr<const Leaf>& leaf, const uint16_t* it) noexcept;
    bool operator==(const Cursor rhs) const { return m_position == rhs.m_position; }

    std::pair<BlobRef, BlobRef> current() const noexcept;
    BlobRef key() const noexcept { return current().first; }
    BlobRef value() const noexcept { return current().second; }
    constexpr explicit operator bool() const noexcept { return m_position.has_value(); }
    // constexpr bool hasValue() const noexcept { return m_position.has_value(); }

private:
    struct Position
    {
        std::shared_ptr<const Leaf> m_leaf;
        uint16_t m_index;

        bool operator==(const Position& rhs) const
        {
            return std::tie(m_leaf, m_index) == std::tie(rhs.m_leaf, rhs.m_index);
        }
    };

    std::optional<Position> m_position;
};

//////////////////////////////////////////////////////////////////////////

class BTree
{
    using InnerNodeStack = std::vector<ConstPageDef<InnerNode>>;

public:
    struct Inserted
    {};

    struct Replaced
    {
        Blob m_beforeValue;
    };

    struct Unchanged
    {
        Cursor m_currentValue;
    };

    using InsertResult = std::variant<Inserted, Replaced, Unchanged>;
    using ReplacePolicy = bool (*)(const BlobRef& newValue, const BlobRef& beforValue);

public:
    BTree(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex = PageIdx::INVALID);

    std::optional<Blob> insert(const Blob& key, const Blob& value)
    {
        auto res = insert(key, value, [](const BlobRef&, const BlobRef&) { return true; });
        auto replaced = std::get_if<Replaced>(&res);
        if (replaced)
            return replaced->m_beforeValue;
        return std::nullopt;
    }

    InsertResult insert(const Blob& key, const Blob& value, ReplacePolicy replacePolicy);
    std::optional<Blob> remove(Blob key);

    Cursor find(const Blob& key) const;
    Cursor begin(const Blob& key) const;
    Cursor next(Cursor cursor) const;

    const std::vector<PageIndex>& getFreePages() const noexcept { return m_freePages; }

private:
    void propagate(InnerNodeStack& stack, const Blob& keyToInsert, PageIndex left, PageIndex right);
    ConstPageDef<Leaf> findLeaf(const Blob& key, InnerNodeStack& stack) const;
    std::shared_ptr<const InnerNode> handleUnderflow(PageDef<InnerNode>& inner, const Blob& key,
                                                     const InnerNodeStack& stack);
    void unlinkLeaveNode(const std::shared_ptr<Leaf>& leaf);

private:
    mutable TypedCacheManager m_cacheManager;
    PageIndex m_rootIndex;
    std::vector<uint32_t> m_freePages;
};

//////////////////////////////////////////////////////////////////////////

}
#endif // BTREE_H
