

#pragma once
#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include "Node.h"
#include "Interval.h"
#include <utility>
#include <memory>
#include <vector>

namespace TxFs
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

    typedef std::pair<std::shared_ptr<Data>, PageIndex> DataPage;
    typedef std::pair<std::shared_ptr<Leaf>, PageIndex> LeafPage;
    typedef std::pair<std::shared_ptr<FileTable>, PageIndex> FileTablePage;
    typedef std::pair<std::shared_ptr<InnerNode>, PageIndex> InnerPage;

public:
    std::shared_ptr<Node> readNode(PageIndex id);
    FileTablePage loadFileTable(PageIndex id);
    FileTablePage newFileTable();
    LeafPage newLeaf(PageIndex prev, PageIndex next);
    InnerPage newInner();
    InnerPage newRoot(const Blob& key, PageIndex left, PageIndex right);
    void setPageDirty(PageIndex id);
    Interval newInterval(size_t maxPages);

    // raw write
    void writePage(const uint8_t* begin, const uint8_t* end, PageIndex page, size_t pageOffset);
    void writePages(const uint8_t* begin, Interval interval);

    // raw read
    uint8_t* readPage(uint8_t* begin, uint8_t* end, PageIndex page, size_t pageOffset);
    uint8_t* readPages(uint8_t* begin, Interval interval);

private:
    std::vector<std::shared_ptr<Node>> m_file;
};

}

#endif
