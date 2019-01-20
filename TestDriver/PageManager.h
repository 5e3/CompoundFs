

#pragma once
#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include "Node.h"
#include "Interval.h"
#include <utility>
#include <memory>
#include <vector>

namespace CompFs
{
class InnerNode;
class Leaf;
class Blob;
class FileTable;

class PageManager
{
public:
    struct Data : Node
    {
        uint8_t m_data[4091];

        Data()
        {
            m_data[0] = 0; // make the compiler happy
        }
    };

    typedef std::pair<std::shared_ptr<Data>, Node::Id> DataPage;
    typedef std::pair<std::shared_ptr<Leaf>, Node::Id> LeafPage;
    typedef std::pair<std::shared_ptr<FileTable>, Node::Id> FileTablePage;
    typedef std::pair<std::shared_ptr<InnerNode>, Node::Id> InnerPage;

public:
    std::shared_ptr<Node> readNode(Node::Id id);
    FileTablePage loadFileTable(Node::Id id);
    FileTablePage newFileTable();
    LeafPage newLeaf(Node::Id prev, Node::Id next);
    InnerPage newInner();
    InnerPage newRoot(const Blob& key, Node::Id left, Node::Id right);
    void pageDirty(Node::Id id);
    Interval newInterval(size_t maxPages);

    // raw write
    void writePage(const uint8_t* begin, const uint8_t* end, Node::Id page, size_t pageOffset);
    void writePages(const uint8_t* begin, Interval interval);
    void finalWritePage(const uint8_t* begin, const uint8_t* end, Node::Id page);

    // raw read
    uint8_t* readPage(uint8_t* begin, uint8_t* end, Node::Id page, size_t pageOffset);
    uint8_t* readPages(uint8_t* begin, Interval interval);

private:
    std::vector<std::shared_ptr<Node>> m_file;
};

}

#endif
