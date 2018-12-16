

#include "stdafx.h"
#include "PageManager.h"
#include "Leaf.h"
#include "InnerNode.h"
#include "FileTable.h"

using namespace CompFs;

std::shared_ptr<Node> PageManager::readNode(Node::Id id)
{
    return m_file.at(id);
}

PageManager::LeafPage PageManager::newLeaf(Node::Id prev, Node::Id next)
{
    std::shared_ptr<Leaf> leaf(new Leaf(prev, next));
    m_file.push_back(leaf);
    return std::make_pair(leaf, m_file.size() - 1);
}

PageManager::InnerPage PageManager::newInner()
{
    std::shared_ptr<InnerNode> inner(new InnerNode);
    m_file.push_back(inner);
    return std::make_pair(inner, m_file.size() - 1);
}

PageManager::InnerPage PageManager::newRoot(const Blob& key, Node::Id left, Node::Id right)
{
    std::shared_ptr<InnerNode> root(new InnerNode(key, left, right));
    m_file.push_back(root);
    return std::make_pair(root, m_file.size() - 1);
}

void PageManager::pageDirty(Node::Id id)
{

}

PageManager::FileTablePage PageManager::loadFileTable(Node::Id id)
{
    std::shared_ptr<Node> node = readNode(id);
    if (node->m_type != NodeType::FileTable)
        throw std::runtime_error("Page is not a FileTable");
    return FileTablePage(std::static_pointer_cast<FileTable>(node), id);
    
}

PageManager::FileTablePage PageManager::newFileTable()
{
    std::shared_ptr<FileTable> fileTable(new FileTable);
    m_file.push_back(fileTable);
    return std::make_pair(fileTable, m_file.size() - 1);
}

IntervalSequence::Interval PageManager::newInterval(size_t maxPages)
{
    return IntervalSequence::Interval(m_file.size(), m_file.size() + std::min(maxPages, size_t(3)));
}


void PageManager::writePage(const uint8_t* begin, const uint8_t* end, Node::Id page, size_t pageOffset)
{
    assert((end - begin + pageOffset) <= 4096);
    if (page == m_file.size())
        m_file.push_back(std::shared_ptr<Node>(new Data));

    std::shared_ptr<Node> node = readNode(page);
    std::copy(begin, end, ((uint8_t*) node.get()) + pageOffset);
}

void PageManager::writePages(const uint8_t* begin, IntervalSequence::Interval interval)
{
    if (interval.first == m_file.size())
    {
        for (Node::Id id = interval.first; id < interval.second; id++)
            m_file.push_back(std::shared_ptr<Node>(new Data));
    }

    for (Node::Id id = interval.first; id < interval.second; id++)
    {
        std::shared_ptr<Node> node = readNode(id);
        std::copy(begin, begin + 4096, (uint8_t*)node.get());
        begin += 4096;
    }
}

void PageManager::finalWritePage(const uint8_t* begin, const uint8_t* end, Node::Id page)
{
    writePage(begin, end, page, 0);
    uint8_t buffer[4096];
    size_t sizeToEndOfPage = sizeof(buffer) - (end - begin);
    std::fill(buffer, buffer + sizeToEndOfPage, 0xfe);
    writePage(buffer, buffer + sizeToEndOfPage, page, end - begin);
}

uint8_t* PageManager::readPage(uint8_t* begin, uint8_t* end, Node::Id page, size_t pageOffset)
{
    std::shared_ptr<Node> node = readNode(page);
    uint8_t* beginPage = ((uint8_t*)node.get()) + pageOffset;
    return std::copy(beginPage, beginPage + (end-begin), begin);
}

uint8_t* PageManager::readPages(uint8_t* begin, IntervalSequence::Interval interval)
{
    for (Node::Id id = interval.first; id < interval.second; id++)
    {
        std::shared_ptr<Node> node = readNode(id);
        uint8_t* beginPage = (uint8_t*)node.get();
        begin = std::copy(beginPage, beginPage + 4096, begin);
    }
    return begin;
}

