

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

class BTree
{
    using InnerNodeStack = std::vector<ConstPageDef<InnerNode>>;

public:
    BTree(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex = PageIdx::INVALID);

    void insert(const Blob& key, const Blob& value);
    std::optional<Blob> find(const Blob& key) const;

private:
    void propagate(InnerNodeStack& stack, const Blob& keyToInsert, PageIndex left, PageIndex right);
    ConstPageDef<Leaf> findLeaf(const Blob& key, InnerNodeStack& stack) const;

private:
    mutable TypedCacheManager m_cacheManager;
    PageIndex m_rootIndex;
};

}
#endif // BTREE_H
