

#pragma once
#ifndef BTREE_H
#define BTREE_H

#include "Node.h"
#include "PageManager.h"

#include <stack>
#include <memory>

namespace CompFs
{

    class BTree
    {

    public:
      typedef PageManager::LeafPage LeafPage;
      typedef PageManager::InnerPage InnerPage;
      typedef std::stack<InnerPage> InnerPageStack;
      static const Node::Id NULL_NODE = Node::INVALID_NODE;

    public:
        BTree(std::shared_ptr<PageManager> pageManager, Node::Id root = NULL_NODE);

        void insert(const Blob& key, const Blob& value);

        bool find(const Blob& key, Blob& value) const;

    private:
        void propagate(InnerPageStack& stack, const Blob& keyToInsert, Node::Id left, Node::Id right);
        LeafPage findLeaf(const Blob& key, InnerPageStack& stack) const;

    private:
        std::shared_ptr<PageManager> m_pageManager;
        Node::Id m_root;

    };

}
#endif //BTREE_H
