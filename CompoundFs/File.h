#pragma once
#ifndef FILE_H
#define FILE_H

#include "Node.h"
#include "FileDescriptor.h"
#include "FileTable.h"
#include "PageManager.h"
#include <algorithm>

namespace TxFs
{

class RawFileWriter
{
public:
    RawFileWriter(std::shared_ptr<PageManager> pageManager, size_t highWaterMark = 250000)
        : m_pageManager(pageManager)
        , m_highWaterMark(highWaterMark)
    {}

    void createNew()
    {
        m_fileDescriptor = FileDescriptor();
        m_pageSequence = IntervalSequence();
        m_fileTable = PageManager::FileTablePage();
    }

    void openAppend(FileDescriptor fileId)
    {
        m_fileDescriptor = fileId;
        m_fileTable = m_pageManager->loadFileTable(fileId.m_last);
        m_fileTable.first->insertInto(m_pageSequence);
    }

    FileDescriptor close()
    {
        pushFileTable();
        if (m_fileTable.first)
            m_fileDescriptor.m_last = m_fileTable.second;
        FileDescriptor res = m_fileDescriptor;
        m_fileDescriptor = FileDescriptor();
        m_fileTable = PageManager::FileTablePage();
        return res;
    }

    void write(const uint8_t* begin, const uint8_t* end)
    {
        size_t blockSize = end - begin;

        // fill last page at max to page boundary
        if (m_fileDescriptor.m_fileSize % 4096)
        {
            size_t pageOffset = size_t(m_fileDescriptor.m_fileSize % 4096);
            const uint8_t* newEndInPage = begin + std::min(4096 - pageOffset, blockSize);
            m_pageManager->writePage(begin, newEndInPage, m_pageSequence.back().end() - 1, pageOffset);
            begin = newEndInPage;
        }

        // write full pages
        size_t pages = (end - begin) / 4096;
        while (pages > 0)
        {
            Interval iv = m_pageManager->newInterval(pages);
            m_pageSequence.pushBack(iv);
            m_pageManager->writePages(begin, iv);
            begin += static_cast<size_t>(iv.length()) * 4096;
            pages -= iv.length();
        }

        // write remaining bytes to partially filled page
        if (end - begin)
        {
            Interval iv = m_pageManager->newInterval(1);
            m_pageSequence.pushBack(iv);
            m_pageManager->finalWritePage(begin, end, iv.end() - 1);
        }
        m_fileDescriptor.m_fileSize += blockSize;

        if (m_pageSequence.size() >= m_highWaterMark)
        {
            pushFileTable();
            m_fileTable.first->insertInto(m_pageSequence);
        }
    }

    void pushFileTable()
    {
        if (m_pageSequence.empty())
            return;

        PageManager::FileTablePage cur;
        if (m_fileTable.first)
        {
            m_fileTable.first->transferFrom(m_pageSequence);
            if (m_fileDescriptor.m_last != PageIdx::INVALID)
            {
                m_pageManager->setPageDirty(m_fileTable.second);
                m_fileDescriptor.m_last = PageIdx::INVALID;
            }
            cur = m_fileTable;
        }
        else
        {
            cur = m_pageManager->newFileTable();
            cur.first->transferFrom(m_pageSequence);
            m_fileDescriptor.m_first = cur.second;
        }

        while (!m_pageSequence.empty())
        {
            PageManager::FileTablePage next = m_pageManager->newFileTable();
            next.first->transferFrom(m_pageSequence);
            cur.first->setNext(next.second);
            cur = next;
        }

        m_fileTable = cur;
    }

    template <class TIter>
    void writeIterator(TIter begin, TIter end)
    {
        write((uint8_t*) &begin[0], (uint8_t*) &begin[end - begin]);
    }

private:
    IntervalSequence m_pageSequence;
    std::shared_ptr<PageManager> m_pageManager;
    PageManager::FileTablePage m_fileTable;
    FileDescriptor m_fileDescriptor;
    size_t m_highWaterMark;
};

//////////////////////////////////////////////////////////////////////////

class RawFileReader
{
public:
    RawFileReader(std::shared_ptr<PageManager> pageManager)
        : m_pageManager(pageManager)
        , m_curFilePos(0)
        , m_fileSize(0)
        , m_nextFileTable(PageIdx::INVALID)
    {}

    void open(FileDescriptor fileId)
    {
        assert(fileId != FileDescriptor());
        m_curFilePos = 0;
        m_fileSize = fileId.m_fileSize;
        PageManager::FileTablePage fileTable = m_pageManager->loadFileTable(fileId.m_first);
        fileTable.first->insertInto(m_pageSequence);
        m_nextFileTable = fileTable.first->getNext();
    }

    Interval nextInterval(uint32_t maxSize)
    {
        if (m_pageSequence.empty())
        {
            if (m_nextFileTable == PageIdx::INVALID)
                return Interval(PageIdx::INVALID, PageIdx::INVALID);

            PageManager::FileTablePage fileTable = m_pageManager->loadFileTable(m_nextFileTable);
            fileTable.first->insertInto(m_pageSequence);
            m_nextFileTable = fileTable.first->getNext();
        }
        return m_pageSequence.popFront(maxSize);
    }

    uint8_t* read(uint8_t* begin, uint8_t* end)
    {
        assert(begin <= end);

        // don't read over the end
        uint64_t blockSize = std::min(uint64_t(end - begin), m_fileSize - m_curFilePos);
        if (blockSize == 0)
            return begin;

        end = begin + blockSize;

        // read the remainder of this page
        if (m_curFilePos % 4096)
        {
            size_t pageOffset = size_t(m_curFilePos % 4096);
            PageIndex pageId = m_pageSequence.front().begin();
            if ((pageOffset + blockSize) >= 4096)
            {
                begin = m_pageManager->readPage(begin, begin + (4096 - pageOffset), pageId, pageOffset);
                nextInterval(1); // remove that page
            }
            else
            {
                begin = m_pageManager->readPage(begin, begin + blockSize, pageId, pageOffset);
            }
        }

        // read full pages
        size_t pages = (end - begin) / 4096;
        while (pages > 0)
        {
            Interval iv = nextInterval((uint32_t) pages);
            begin = m_pageManager->readPages(begin, iv);
            pages -= iv.length();
        }

        // partially read the next page
        if (end - begin)
        {
            PageIndex pageId = nextInterval(0).begin();
            begin = m_pageManager->readPage(begin, end, pageId, 0);
        }

        m_curFilePos += blockSize;
        return begin;
    }

    uint64_t bytesLeft() const { return m_fileSize - m_curFilePos; }

    template <class TIter>
    uint8_t* readIterator(TIter begin, TIter end)
    {
        return read((uint8_t*) &begin[0], (uint8_t*) &begin[end - begin]);
    }

private:
    IntervalSequence m_pageSequence;
    std::shared_ptr<PageManager> m_pageManager;

    uint64_t m_curFilePos;
    uint64_t m_fileSize;
    uint32_t m_nextFileTable;
};

}

#endif
