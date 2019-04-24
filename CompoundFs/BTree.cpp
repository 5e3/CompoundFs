

#include "BTree.h"
#include "Leaf.h"
#include "InnerNode.h"

#include <algorithm>
#include <assert.h>

using namespace TxFs;

//////////////////////////////////////////////////////////////////////////
Cursor::Cursor(const std::shared_ptr<const Leaf>& leaf, const uint16_t* it) noexcept
    : m_position({ leaf, uint16_t(it - leaf->beginTable()) })
{}

std::pair<ByteStringView, ByteStringView> Cursor::current() const noexcept
{
    const auto& [leaf, index] = *m_position;
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

BTree::InsertResult BTree::insert(const ByteString& key, const ByteString& value, ReplacePolicy replacePolicy)
{
    InnerNodeStack stack;
    stack.reserve(5);

    auto leafDef = m_cacheManager.makePageWritable(findLeaf(key, stack));
    auto it = leafDef.m_page->find(key);
    InsertResult result;

    if (it != leafDef.m_page->endTable())
    {
        // there is already an entry for that key
        ByteStringView ventry = leafDef.m_page->getValue(it);
        if (!replacePolicy(ventry))
            return Unchanged { Cursor(leafDef.m_page, it) };

        result = Replaced { ventry };
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

void BTree::unlinkLeaveNode(const std::shared_ptr<Leaf>& leaf)
{
    if (leaf->getPrev() != PageIdx::INVALID)
    {
        auto left = m_cacheManager.loadPage<Leaf>(leaf->getPrev());
        m_cacheManager.makePageWritable(left).m_page->setNext(leaf->getNext());
    }
    if (leaf->getNext() != PageIdx::INVALID)
    {
        auto right = m_cacheManager.loadPage<Leaf>(leaf->getNext());
        m_cacheManager.makePageWritable(right).m_page->setPrev(leaf->getPrev());
    }
}

std::shared_ptr<const InnerNode> BTree::handleUnderflow(PageDef<InnerNode>& inner, const ByteString& key,
                                                        const InnerNodeStack& stack)
{
    assert(inner.m_page->nofItems() == 1);
    assert(stack.size() >= 2);

    auto parent = m_cacheManager.makePageWritable(*(stack.end() - 2));
    assert(parent.m_page->findPage(key) == inner.m_index);

    auto it = parent.m_page->findKey(key);
    assert(parent.m_page->getLeft(it) == inner.m_index || parent.m_page->getRight(it) == inner.m_index);
    auto left = m_cacheManager.makePageWritable(m_cacheManager.loadPage<InnerNode>(parent.m_page->getLeft(it)));
    auto right = m_cacheManager.loadPage<InnerNode>(parent.m_page->getRight(it));
    auto parentKey = parent.m_page->getKey(it);
    if (!left.m_page->canMergeWith(*right.m_page, parentKey))
    {
        auto rightPage = m_cacheManager.makePageWritable(right).m_page;
        auto newParentKey = InnerNode::redistribute(*left.m_page, *rightPage, parentKey);
        parent.m_page->remove(parentKey);
        parent.m_page->insert(newParentKey, right.m_index);
        return nullptr;
    }

    left.m_page->mergeWith(*right.m_page, parentKey);
    m_freePages.push_back(right.m_index);
    return right.m_page;
}

std::optional<ByteString> BTree::remove(ByteString key)
{
    InnerNodeStack stack;
    stack.reserve(5);

    auto leafDef = findLeaf(key, stack);
    auto it = leafDef.m_page->find(key);
    if (it == leafDef.m_page->endTable())
        return std::nullopt;

    ByteString beforeValue = leafDef.m_page->getValue(it);
    auto leaf = m_cacheManager.makePageWritable(leafDef).m_page;
    leaf->remove(key);
    if (leaf->nofItems() > 0)
        return beforeValue;

    unlinkLeaveNode(leaf);
    m_freePages.push_back(leafDef.m_index);
    while (stack.size() > 0)
    {
        auto inner = m_cacheManager.makePageWritable(stack.back());
        inner.m_page->remove(key);

        if (inner.m_page->nofItems() > 1)
            return beforeValue;
        else if (inner.m_page->nofItems() == 0)
        {
            // must be the root page
            assert(m_rootIndex == inner.m_index);
            m_freePages.push_back(m_rootIndex);
            m_rootIndex = inner.m_page->getLeft(inner.m_page->beginTable());
            return beforeValue;
        }
        else if (stack.size() == 1)
            return beforeValue;

        auto freePage = handleUnderflow(inner, key, stack);
        if (!freePage)
            return beforeValue;

        key = freePage->getKey(freePage->beginTable());
        stack.pop_back();
    }

    return beforeValue;
}

ConstPageDef<Leaf> BTree::findLeaf(const ByteString& key, InnerNodeStack& stack) const
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

void BTree::propagate(InnerNodeStack& stack, const ByteString& keyToInsert, PageIndex left, PageIndex right)
{
    ByteString key = keyToInsert;
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

Cursor BTree::find(const ByteString& key) const
{
    InnerNodeStack stack;
    stack.reserve(5);

    auto leafDef = findLeaf(key, stack);
    auto it = leafDef.m_page->find(key);
    if (it == leafDef.m_page->endTable())
        return Cursor();
    return Cursor(leafDef.m_page, it);
}

Cursor BTree::begin(const ByteString& key) const
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
