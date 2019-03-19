

#include "BTree.h"
#include "Leaf.h"
#include "InnerNode.h"

#include <algorithm>
#include <assert.h>

using namespace TxFs;

//////////////////////////////////////////////////////////////////////////
constexpr Cursor::Cursor(const std::shared_ptr<const Leaf>& leaf, const uint16_t* it)
    : m_position({ leaf, uint16_t(it - leaf->beginTable()) })
{}



std::pair<BlobRef, BlobRef> Cursor::current() const
{
    const auto&[leaf, index] = *m_position;
    auto it = leaf->beginTable() + index;
    return std::make_pair(leaf->getKey(it), leaf->getValue(it));
}

//////////////////////////////////////////////////////////////////////////

BTree::BTree(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex)
    : m_cacheManager(cacheManager)
    , m_rootIndex(rootIndex)
{
    if (m_rootIndex == PageIdx::INVALID)
        m_rootIndex = m_cacheManager.newPage<Leaf>(PageIdx::INVALID, PageIdx::INVALID).m_index;
}

BTree::InsertResult BTree::insert(const Blob& key, const Blob& value, ReplacePolicy replacePolicy)
{
    InnerNodeStack stack;
    stack.reserve(5);

    auto leafDef = m_cacheManager.makePageWritable(findLeaf(key, stack));
    auto it = leafDef.m_page->find(key);
    InsertResult result;

    if (it != leafDef.m_page->endTable())
    {
        // there is already an entry for that key
        BlobRef ventry = leafDef.m_page->getValue(it);
        if (!replacePolicy(value, ventry))
            return Unchanged{ Cursor(leafDef.m_page, it) };

        result = Replaced{ ventry };
        if (ventry.size() == value.size())
        {
            // replace at the same position
            std::copy(value.begin(), value.end(), ventry.begin());
            return result;
        }
        leafDef.m_page->remove(key);
    }

    if (leafDef.m_page->hasSpace(key, value))
    {
        leafDef.m_page->insert(key, value);
        return result;
    }

    // link rightLeaf to the right hand-side of leafDef
    auto rightLeaf = m_cacheManager.newPage<Leaf>(leafDef.m_index, leafDef.m_page->getNext());
    leafDef.m_page->setNext(rightLeaf.m_index);

    // split and move up
    leafDef.m_page->split(rightLeaf.m_page.get(), key, value);
    propagate(stack, rightLeaf.m_page->getLowestKey(), leafDef.m_index, rightLeaf.m_index);
    return result;
}

std::optional<Blob> BTree::remove(const Blob& key)
{
    InnerNodeStack stack;
    stack.reserve(5);

    auto leafDef = findLeaf(key, stack);
    auto it = leafDef.m_page->find(key);
    if (it == leafDef.m_page->endTable())
        return std::nullopt;

    Blob beforeValue = leafDef.m_page->getValue(it);
    auto leaf = m_cacheManager.makePageWritable(leafDef).m_page;
    leaf->remove(key);
    if (!leaf->empty())
        return beforeValue;

    auto freePage = leafDef.m_index;
    while (stack.size() > 1)
    {
        m_freePages.push_back(freePage);
        auto inner = m_cacheManager.makePageWritable(stack.back());
        inner.m_page->remove(key);
        if (!inner.m_page->empty())
            return beforeValue;

        freePage = inner.m_index;
        stack.pop_back();
    }

    return beforeValue;
}


Cursor BTree::find(const Blob& key) const
{
    InnerNodeStack stack;
    stack.reserve(5);

    auto leafDef = findLeaf(key, stack);
    auto it = leafDef.m_page->find(key);
    if (it == leafDef.m_page->endTable())
        return Cursor();
    return Cursor(leafDef.m_page, it);
}

ConstPageDef<Leaf> BTree::findLeaf(const Blob& key, InnerNodeStack& stack) const
{
    PageIndex id = m_rootIndex;
    while (true)
    {
        auto nodeDef = m_cacheManager.loadPage<Node>(id);
        if (nodeDef.m_page->m_type == NodeType::Leaf)
            return ConstPageDef<Leaf>(std::static_pointer_cast<const Leaf>(nodeDef.m_page), id);
        stack.push_back(ConstPageDef<InnerNode>(std::static_pointer_cast<const InnerNode>(nodeDef.m_page), id));
        id = stack.back().m_page->findPage(key);
    }
}

void BTree::propagate(InnerNodeStack& stack, const Blob& keyToInsert, PageIndex left, PageIndex right)
{
    Blob key = keyToInsert;
    while (!stack.empty())
    {
        auto inner = m_cacheManager.makePageWritable(stack.back());
        if (inner.m_page->hasSpace(key))
        {
            inner.m_page->insert(key, right);
            return;
        }

        auto rightInner = m_cacheManager.newPage<InnerNode>();
        key = inner.m_page->split(rightInner.m_page.get(), key, right);
        left = inner.m_index;
        right = rightInner.m_index;
        stack.pop_back();
    }

    m_rootIndex = m_cacheManager.newPage<InnerNode>(key, left, right).m_index;
}

Cursor BTree::begin(const Blob& key) const
{
    InnerNodeStack stack;
    stack.reserve(5);

    auto leafDef = findLeaf(key, stack);
    auto it = leafDef.m_page->lowerBound(key);
    if (it == leafDef.m_page->endTable())
        return Cursor();

    return Cursor(leafDef.m_page, it);
}

Cursor BTree::next(Cursor cursor) const
{
    if (!cursor)
        return cursor;

    const auto& [leaf, index] = *cursor.m_position;
    if (leaf->beginTable() + index + 1 < leaf->endTable())
        return Cursor(leaf, leaf->beginTable() + index + 1);

    if (leaf->getNext() == PageIdx::INVALID)
        return Cursor();

    const auto& nextLeaf = m_cacheManager.loadPage<Leaf>(leaf->getNext()).m_page;
    return Cursor(nextLeaf, nextLeaf->beginTable());
}

