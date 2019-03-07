

#pragma once
#ifndef BTREE_H
#define BTREE_H

#include "PageDef.h"
#include "TypedCacheManager.h"
#include "Blob.h"

#include <stack>
#include <memory>

namespace TxFs
{
class Leaf;
class InnerNode;

class BTree
{

public:
    using LeafDef = PageDef<Leaf>;
    using InnerNodeDef = PageDef<InnerNode>;
    using InnerNodeStack = std::stack<ConstPageDef<InnerNode>>;

public:
    BTree(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex = PageIdx::INVALID);

    void insert(const Blob& key, const Blob& value);
    bool find(const Blob& key, Blob& value) const;

private:
    void propagate(InnerNodeStack& stack, const Blob& keyToInsert, PageIndex left, PageIndex right);
    ConstPageDef<Leaf> findLeaf(const Blob& key, InnerNodeStack& stack) const;

private:
    mutable TypedCacheManager m_cacheManager;
    PageIndex m_rootIndex;
};

}
#endif // BTREE_H
