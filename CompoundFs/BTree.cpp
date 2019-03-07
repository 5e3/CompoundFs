

#include "BTree.h"
#include "Leaf.h"
#include "InnerNode.h"

#include <algorithm>
#include <assert.h>

using namespace TxFs;

BTree::BTree(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex)
    : m_cacheManager(cacheManager)
    , m_rootIndex(rootIndex)
{
    if (m_rootIndex == PageIdx::INVALID)
        m_rootIndex = m_cacheManager.newPage<Leaf>(PageIdx::INVALID, PageIdx::INVALID).m_index;
}

void BTree::insert(const Blob& key, const Blob& value)
{
    InnerNodeStack stack;
    auto leafDef = m_cacheManager.makePageWritable(findLeaf(key, stack));
    auto it = leafDef.m_page->find(key);
    if (it != leafDef.m_page->endTable())
    {
        // there is already an entry for that key
        Blob ventry = leafDef.m_page->getValue(it);
        if (ventry.size() == value.size())
        {
            // replace at the same position
            std::copy(value.begin(), value.end(), ventry.begin());
            return;
        }
        leafDef.m_page->remove(key);
    }

    if (leafDef.m_page->hasSpace(key, value))
    {
        leafDef.m_page->insert(key, value);
        return;
    }

    auto rightLeaf = m_cacheManager.newPage<Leaf>(leafDef.m_index, leafDef.m_page->getNext());
    leafDef.m_page->setNext(rightLeaf.m_index);
    leafDef.m_page->split(rightLeaf.m_page.get(), key, value);
    propagate(stack, rightLeaf.m_page->getLowestKey(), leafDef.m_index, rightLeaf.m_index);
}

bool BTree::find(const Blob& key, Blob& value) const
{
    InnerNodeStack stack;
    auto leafDef = findLeaf(key, stack);
    auto it = leafDef.m_page->find(key);
    if (it == leafDef.m_page->endTable())
        return false;

    value.assign(leafDef.m_page->getValue(it).begin());
    return true;
}

ConstPageDef<Leaf> BTree::findLeaf(const Blob& key, InnerNodeStack& stack) const
{
    PageIndex id = m_rootIndex;
    while (true)
    {
        auto nodeDef = m_cacheManager.loadPage<Node>(id);
        if (nodeDef.m_page->m_type == NodeType::Leaf)
            return ConstPageDef<Leaf>(std::static_pointer_cast<const Leaf>(nodeDef.m_page), id);
        stack.push(ConstPageDef<InnerNode>(std::static_pointer_cast<const InnerNode>(nodeDef.m_page), id));
        id = stack.top().m_page->findPage(key);
    }
}

void BTree::propagate(InnerNodeStack& stack, const Blob& keyToInsert, PageIndex left, PageIndex right)
{
    Blob key = keyToInsert;
    Blob keyMiddle;
    while (!stack.empty())
    {
        auto inner = m_cacheManager.makePageWritable(stack.top());
        if (inner.m_page->hasSpace(key))
        {
            inner.m_page->insert(key, right);
            return;
        }

        auto rightInner = m_cacheManager.newPage<InnerNode>();
        inner.m_page->split(rightInner.m_page.get(), keyMiddle, key, right);
        key = keyMiddle;
        left = inner.m_index;
        right = rightInner.m_index;
        stack.pop();
    }

    m_rootIndex = m_cacheManager.newPage<InnerNode>(key, left, right).m_index;
}
