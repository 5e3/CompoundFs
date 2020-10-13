 

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

//////////////////////////////////////////////////////////////////////////

class BTree final
{
    using InnerNodeStack = std::vector<ConstPageDef<InnerNode>>;
    struct KeyInserter;

public:
    class Cursor;
    struct Inserted;
    struct Replaced;
    struct Unchanged;
    struct NotFound;
    using InsertResult = std::variant<Inserted, Replaced, Unchanged>;
    using RenameResult = std::variant<NotFound, Inserted, Unchanged>;
    using ReplacePolicy = bool (*)(ByteStringView beforValue);

public:
    BTree(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex = PageIdx::INVALID);
    BTree(BTree&&) = default;
    BTree& operator=(BTree&&) = default;

    std::optional<ByteString> insert(ByteStringView key, ByteStringView value);
    InsertResult insert(ByteStringView key, ByteStringView value, ReplacePolicy replacePolicy);
    RenameResult rename(ByteStringView oldKey, ByteStringView newKey);
    std::optional<ByteString> remove(ByteStringView key);

    Cursor find(ByteStringView key) const;
    Cursor begin(ByteStringView key) const;
    Cursor next(Cursor cursor) const;

    const std::vector<PageIndex>& getFreePages() const noexcept { return m_freePages; }

private:
    void propagate(InnerNodeStack& stack, ByteStringView keyToInsert, PageIndex left, PageIndex right);
    ConstPageDef<Leaf> findLeaf(ByteStringView key, InnerNodeStack& stack) const;
    std::shared_ptr<const InnerNode> handleUnderflow(PageDef<InnerNode>& inner, ByteStringView key,
                                                     const InnerNodeStack& stack);
    void unlinkLeaveNode(const std::shared_ptr<Leaf>& leaf);
    void growTree(ByteStringView keyToInsert, bool leftRightIsLeaf, PageIndex left, PageIndex right);

private:
    mutable TypedCacheManager m_cacheManager;
    PageIndex m_rootIndex;
    std::vector<uint32_t> m_freePages;
};

//////////////////////////////////////////////////////////////////////////

class BTree::Cursor final
{
    friend class BTree;

public:
    constexpr Cursor() noexcept = default;
    Cursor(const std::shared_ptr<const Leaf>& leaf, const uint16_t* it) noexcept;

    constexpr bool operator==(const Cursor& rhs) const noexcept { return m_position == rhs.m_position; }

    std::pair<ByteStringView, ByteStringView> current() const;
    ByteStringView key() const { return current().first; }
    ByteStringView value() const { return current().second; }
    constexpr explicit operator bool() const noexcept { return m_position.has_value(); }

private:
    struct Position
    {
        std::shared_ptr<const Leaf> m_leaf;
        uint16_t m_index;

        constexpr bool operator==(const Position& rhs) const noexcept
        {
            return std::tie(m_leaf, m_index) == std::tie(rhs.m_leaf, rhs.m_index);
        }
    };

    std::optional<Position> m_position;
};

//////////////////////////////////////////////////////////////////////////

struct BTree::Inserted
{};

//////////////////////////////////////////////////////////////////////////

struct BTree::Replaced
{
    ByteString m_beforeValue;
};

//////////////////////////////////////////////////////////////////////////

struct BTree::Unchanged
{
    Cursor m_currentValue;
};
//////////////////////////////////////////////////////////////////////////

struct BTree::NotFound
{};



}
#endif // BTREE_H
