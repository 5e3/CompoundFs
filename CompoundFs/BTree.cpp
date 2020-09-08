

#include "BTree.h"
#include "Leaf.h"
#include "InnerNode.h"

#include <algorithm>
#include <assert.h>

using namespace TxFs;

//////////////////////////////////////////////////////////////////////////

BTree::BTree(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex)
    : m_cacheManager(cacheManager)
    , m_rootIndex(rootIndex)
{
    if (m_rootIndex == PageIdx::INVALID)
        m_rootIndex = m_cacheManager.newPage<Leaf>(PageIdx::INVALID, PageIdx::INVALID).m_index;
}

struct BTree::KeyInserter
{
    ByteStringView m_key;
    ByteStringView m_value;
    BTree* m_btree;
    InnerNodeStack m_stack;
    ConstPageDef<Leaf> m_constLeafDef;

    const uint16_t* findExact()
    {
        m_constLeafDef = m_btree->findLeaf(m_key, m_stack);
        auto it = m_constLeafDef.m_page->find(m_key);

        return it == m_constLeafDef.m_page->endTable() ? nullptr : it;
    }

    InsertResult insertExistingKey(ReplacePolicy replacePolicy, const uint16_t* it)
    {
        ByteStringView ventry = m_constLeafDef.m_page->getValue(it);
        if (!replacePolicy(ventry))
            return Unchanged { Cursor(m_constLeafDef.m_page, it) };

        InsertResult res = Replaced { ventry };
        if (ventry == m_value)
            return res;

        auto leafDef = m_btree->m_cacheManager.makePageWritable(m_constLeafDef);
        if (ventry.size() == m_value.size())
        {
            // replace at the same position
            auto ventryPtr = const_cast<uint8_t*>(ventry.data());
            std::copy(m_value.data(), m_value.end(), ventryPtr);
            return res;
        }

        leafDef.m_page->remove(m_key);
        insertNewKey(leafDef);
        return res;
    }

    void insertNewKey(PageDef<Leaf> leafDef)
    {
        if (leafDef.m_page->hasSpace(m_key, m_value))
        {
            leafDef.m_page->insert(m_key, m_value);
            return;
        }

        // link rightLeaf to the right hand-side of leafDef
        auto rightLeaf = m_btree->m_cacheManager.newPage<Leaf>(leafDef.m_index, leafDef.m_page->getNext());
        leafDef.m_page->setNext(rightLeaf.m_index);

        // split and move up
        leafDef.m_page->split(rightLeaf.m_page.get(), m_key, m_value);
        m_btree->propagate(m_stack, rightLeaf.m_page->getLowestKey(), leafDef.m_index, rightLeaf.m_index);
    }

    void insertNewKey() { insertNewKey(m_btree->m_cacheManager.makePageWritable(m_constLeafDef)); }
};

BTree::InsertResult BTree::insert(ByteStringView key, ByteStringView value, ReplacePolicy replacePolicy)
{
    KeyInserter keyInserter { key, value, this };
    auto it = keyInserter.findExact();
    if (it)
        return keyInserter.insertExistingKey(replacePolicy, it);

    keyInserter.insertNewKey();
    return Inserted {};
}

std::optional<ByteString> BTree::insert(ByteStringView key, ByteStringView value)
{
    auto res = insert(key, value, [](ByteStringView) { return true; });
    auto replaced = std::get_if<Replaced>(&res);
    if (replaced)
        return replaced->m_beforeValue;
    return std::nullopt;
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

std::shared_ptr<const InnerNode> BTree::handleUnderflow(PageDef<InnerNode>& inner, ByteStringView key,
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

std::optional<ByteString> BTree::remove(ByteStringView key)
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
    while (!stack.empty())
    {
        auto inner = m_cacheManager.makePageWritable(stack.back());
        auto innerPage = inner.m_page;
        innerPage->remove(key);

        if (innerPage->nofItems() > 1)
            return beforeValue;
        else if (innerPage->nofItems() == 0)
        {
            // must be the root page
            assert(m_rootIndex == inner.m_index);

            auto leaf = m_cacheManager.loadPage<Leaf>(innerPage->getLeft(innerPage->beginTable()));
            auto root = new (innerPage.get()) Leaf(*leaf.m_page);
            m_freePages.push_back(leaf.m_index);
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

ConstPageDef<Leaf> BTree::findLeaf(ByteStringView key, InnerNodeStack& stack) const
{
    PageIndex id = m_rootIndex;
    while (true)
    {
        auto nodeDef = m_cacheManager.loadPage<Node>(id);
        if (nodeDef.m_page->m_type == NodeType::Leaf)
            return ConstPageDef<Leaf>(std::static_pointer_cast<const Leaf>(nodeDef.m_page), id);
        stack.emplace_back(std::static_pointer_cast<const InnerNode>(nodeDef.m_page), id);
        id = stack.back().m_page->findPage(key);
    }
}

void BTree::propagate(InnerNodeStack& stack, ByteStringView keyToInsert, PageIndex left, PageIndex right)
{
    ByteString key = keyToInsert;
    bool leftRightIsLeaf = stack.empty();
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

    growTree(key, leftRightIsLeaf, left, right);
    //m_rootIndex = m_cacheManager.newPage<InnerNode>(key, left, right).m_index;
}

void TxFs::BTree::growTree(ByteStringView key, bool leftRightIsLeaf, PageIndex left, PageIndex right)
{
    assert(m_rootIndex == left);
    if (leftRightIsLeaf)
    {
        auto pageDef = m_cacheManager.newPage<Leaf>();
        auto leftDef = m_cacheManager.makePageWritable(m_cacheManager.loadPage<Leaf>(left));
        auto rightDef = m_cacheManager.makePageWritable(m_cacheManager.loadPage<Leaf>(right));
        *pageDef.m_page = *leftDef.m_page; 
        rightDef.m_page->setPrev(pageDef.m_index);
        auto root = new (leftDef.m_page.get()) InnerNode(key, pageDef.m_index, right);
        return;
    }

    auto pageDef = m_cacheManager.newPage<InnerNode>();
    auto leftDef = m_cacheManager.makePageWritable(m_cacheManager.loadPage<InnerNode>(left));
    *pageDef.m_page = *leftDef.m_page;
    auto root = new (leftDef.m_page.get()) InnerNode(key, pageDef.m_index, right);



}

BTree::Cursor BTree::find(ByteStringView key) const
{
    InnerNodeStack stack;
    stack.reserve(5);

    auto leafDef = findLeaf(key, stack);
    auto it = leafDef.m_page->find(key);
    if (it == leafDef.m_page->endTable())
        return Cursor();
    return Cursor(leafDef.m_page, it);
}

BTree::Cursor BTree::begin(ByteStringView key) const
{
    InnerNodeStack stack;
    stack.reserve(5);

    auto leafDef = findLeaf(key, stack);
    auto it = leafDef.m_page->lowerBound(key);
    if (it == leafDef.m_page->endTable())
        return Cursor();

    return Cursor(leafDef.m_page, it);
}

BTree::Cursor BTree::next(Cursor cursor) const
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

//////////////////////////////////////////////////////////////////////////

BTree::Cursor::Cursor(const std::shared_ptr<const Leaf>& leaf, const uint16_t* it) noexcept
    : m_position({ leaf, uint16_t(it - leaf->beginTable()) })
{}

std::pair<ByteStringView, ByteStringView> BTree::Cursor::current() const
{
    const auto& [leaf, index] = *m_position;
    auto it = leaf->beginTable() + index;
    return std::make_pair(leaf->getKey(it), leaf->getValue(it));
}

