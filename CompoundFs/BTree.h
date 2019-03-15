

#pragma once
#ifndef BTREE_H
#define BTREE_H

#include "PageDef.h"
#include "TypedCacheManager.h"
#include "Blob.h"

#include <vector>
#include <memory>
#include <optional>

namespace TxFs
{
class Leaf;
class InnerNode;
class Cursor;

//////////////////////////////////////////////////////////////////////////

class BTree
{
    using InnerNodeStack = std::vector<ConstPageDef<InnerNode>>;

public:
    BTree(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex = PageIdx::INVALID);

    void insert(const Blob& key, const Blob& value);
    std::optional<Blob> find(const Blob& key) const;

    Cursor begin(const Blob& key) const;
    Cursor next(Cursor cursor) const;


private:
    void propagate(InnerNodeStack& stack, const Blob& keyToInsert, PageIndex left, PageIndex right);
    ConstPageDef<Leaf> findLeaf(const Blob& key, InnerNodeStack& stack) const;

private:
    mutable TypedCacheManager m_cacheManager;
    PageIndex m_rootIndex;
};

//////////////////////////////////////////////////////////////////////////

class Cursor
{
    friend class BTree;

public:
    Cursor() = default;
    Cursor(const std::shared_ptr<const Leaf>& leaf, uint16_t index)
        : m_position({ leaf, index })
    {}

    bool done() const { return !m_position.has_value(); }
    std::pair<BlobRef, BlobRef> current() const;

private:
    struct Position
    {
        std::shared_ptr<const Leaf> m_leaf;
        uint16_t m_index;
    };

    std::optional<Position> m_position;
};

}
#endif // BTREE_H
