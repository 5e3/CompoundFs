

#pragma once
#ifndef BTREE_H
#define BTREE_H

#include "PageDef.h"
#include "TypedCacheManager.h"
#include "ByteString.h"

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

    std::pair<ByteStringView, ByteStringView> current() const noexcept;
    ByteStringView key() const noexcept { return current().first; }
    ByteStringView value() const noexcept { return current().second; }
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
        ByteString m_beforeValue;
    };

    struct Unchanged
    {
        Cursor m_currentValue;
    };

    using InsertResult = std::variant<Inserted, Replaced, Unchanged>;
    using ReplacePolicy = bool (*)(const ByteStringView& beforValue);

public:
    BTree(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex = PageIdx::INVALID);

    std::optional<ByteString> insert(const ByteString& key, const ByteString& value)
    {
        auto res = insert(key, value, [](const ByteStringView&) { return true; });
        auto replaced = std::get_if<Replaced>(&res);
        if (replaced)
            return replaced->m_beforeValue;
        return std::nullopt;
    }

    InsertResult insert(const ByteString& key, const ByteString& value, ReplacePolicy replacePolicy);
    std::optional<ByteString> remove(ByteString key);

    Cursor find(const ByteString& key) const;
    Cursor begin(const ByteString& key) const;
    Cursor next(Cursor cursor) const;

    const std::vector<PageIndex>& getFreePages() const noexcept { return m_freePages; }

private:
    void propagate(InnerNodeStack& stack, const ByteString& keyToInsert, PageIndex left, PageIndex right);
    ConstPageDef<Leaf> findLeaf(const ByteString& key, InnerNodeStack& stack) const;
    std::shared_ptr<const InnerNode> handleUnderflow(PageDef<InnerNode>& inner, const ByteString& key,
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
