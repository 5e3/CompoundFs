

#include "Leaf.h"
#include "InnerNode.h"
#include "BTree.h"

#include <algorithm>
#include <assert.h>

using namespace TxFs;

BTree::BTree(std::shared_ptr<PageManager> pageManager, PageIndex root)
    : m_pageManager(pageManager)
    , m_root(root)
{
    if (m_root == NULL_NODE)
        m_root = m_pageManager->newLeaf(NULL_NODE, NULL_NODE).second;
}

void BTree::insert(const Blob& key, const Blob& value)
{
    InnerPageStack stack;
    LeafPage leafPage = findLeaf(key, stack);
    m_pageManager->setPageDirty(leafPage.second);
    auto it = leafPage.first->find(key);
    if (it != leafPage.first->endTable())
    {
        // there is already an entry for that key
        Blob ventry = leafPage.first->getValue(it);
        if (ventry.size() == value.size())
        {
            // replace at the same position
            std::copy(value.begin(), value.end(), ventry.begin());
            return;
        }
        leafPage.first->remove(key);
    }

    if (leafPage.first->hasSpace(key, value))
    {
        leafPage.first->insert(key, value);
        return;
    }

    LeafPage rightLeaf = m_pageManager->newLeaf(leafPage.second, leafPage.first->getNext());
    leafPage.first->setNext(rightLeaf.second);
    leafPage.first->split(rightLeaf.first.get(), key, value);
    propagate(stack, rightLeaf.first->getLowestKey(), leafPage.second, rightLeaf.second);
}

bool BTree::find(const Blob& key, Blob& value) const
{
    InnerPageStack stack;
    LeafPage leafPage = findLeaf(key, stack);
    auto it = leafPage.first->find(key);
    if (it == leafPage.first->endTable())
        return false;

    value.assign(leafPage.first->getValue(it).begin());
    return true;
}

BTree::LeafPage BTree::findLeaf(const Blob& key, InnerPageStack& stack) const
{
    PageIndex id = m_root;
    while (true)
    {
        std::shared_ptr<Node> node = m_pageManager->readNode(id);
        if (node->m_type == NodeType::Leaf)
            return std::make_pair(std::static_pointer_cast<Leaf>(node), id);
        std::shared_ptr<InnerNode> inner = std::static_pointer_cast<InnerNode>(node);
        stack.push(std::make_pair(inner, id));
        id = inner->findPage(key);
    }
}

void BTree::propagate(InnerPageStack& stack, const Blob& keyToInsert, PageIndex left, PageIndex right)
{
    Blob key = keyToInsert;
    Blob keyMiddle;
    while (!stack.empty())
    {
        InnerPage inner = stack.top();
        m_pageManager->setPageDirty(inner.second);
        if (inner.first->hasSpace(key))
        {
            inner.first->insert(key, right);
            return;
        }

        InnerPage rightInner = m_pageManager->newInner();
        inner.first->split(rightInner.first.get(), keyMiddle, key, right);
        key = keyMiddle;
        left = inner.second;
        right = rightInner.second;
        stack.pop();
    }

    InnerPage root = m_pageManager->newRoot(key, left, right);
    m_root = root.second;
}
