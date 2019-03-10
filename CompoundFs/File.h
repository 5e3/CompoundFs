#pragma once
#ifndef FILE_H
#define FILE_H

#include "Node.h"
#include "FileDescriptor.h"
#include "FileTable.h"
#include "TypedCacheManager.h"
#include "PageDef.h"
#include "RawFileInterface.h"
#include <algorithm>

namespace TxFs
{

class RawFileWriter
{
public:
    RawFileWriter(const std::shared_ptr<CacheManager>& cacheManager, size_t highWaterMark = 250000)
        : m_cacheManager(cacheManager)
        , m_highWaterMark(highWaterMark)
    {
        assert(m_highWaterMark > 0);
    }

    void createNew()
    {
        m_fileDescriptor = FileDescriptor();
        m_pageSequence = IntervalSequence();
        m_fileTable = ConstPageDef<FileTable>();
    }

    void openAppend(FileDescriptor fileId)
    {
        m_fileDescriptor = fileId;
        m_fileTable = m_cacheManager.loadPage<FileTable>(fileId.m_last);
        m_fileTable.m_page->insertInto(m_pageSequence);
    }

    FileDescriptor close()
    {
        pushFileTable();
        if (m_fileTable.m_page)
            m_fileDescriptor.m_last = m_fileTable.m_index;
        FileDescriptor res = m_fileDescriptor;
        m_fileDescriptor = FileDescriptor();
        m_fileTable = ConstPageDef<FileTable>();
        return res;
    }

    void write(const uint8_t* begin, const uint8_t* end)
    {
        const size_t blockSize = end - begin;

        // fill last page at max to page boundary
        if (m_fileDescriptor.m_fileSize % 4096)
        {
            size_t pageOffset = size_t(m_fileDescriptor.m_fileSize % 4096);
            const uint8_t* newEndInPage = begin + std::min(4096 - pageOffset, blockSize);
            m_cacheManager.getRawFileInterface()->writePage(m_pageSequence.back().end() - 1, pageOffset, begin,
                                                            newEndInPage);
            begin = newEndInPage;
        }

        // write full pages
        size_t pages = (end - begin) / 4096;
        while (pages > 0)
        {
            Interval iv = m_cacheManager.allocatePageInterval(pages);
            m_pageSequence.pushBack(iv);
            m_cacheManager.getRawFileInterface()->writePages(iv, begin);
            begin += static_cast<size_t>(iv.length()) * 4096;
            pages -= iv.length();
        }

        // write remaining bytes to partially filled page
        if (end - begin)
        {
            Interval iv = m_cacheManager.allocatePageInterval(1);
            m_pageSequence.pushBack(iv);
            m_cacheManager.getRawFileInterface()->writePage(iv.begin(), 0, begin, end);
        }
        m_fileDescriptor.m_fileSize += blockSize;

        if (m_pageSequence.size() >= m_highWaterMark)
        {
            pushFileTable();
            m_fileTable.m_page->insertInto(m_pageSequence);
        }
    }

    void pushFileTable()
    {
        if (m_pageSequence.empty())
            return;

        // create a FileTable page if we never had before and fill it
        PageDef<FileTable> cur
            = m_fileTable.m_page ? m_cacheManager.makePageWritable(m_fileTable) : m_cacheManager.newPage<FileTable>();
        cur.m_page->transferFrom(m_pageSequence);

        // adjust file descriptor if its still a default one
        if (m_fileDescriptor.m_first == PageIdx::INVALID)
            m_fileDescriptor.m_first = cur.m_index;

        while (!m_pageSequence.empty())
        {
            PageDef<FileTable> next = m_cacheManager.newPage<FileTable>();
            next.m_page->transferFrom(m_pageSequence);
            cur.m_page->setNext(next.m_index);
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
    TypedCacheManager m_cacheManager;
    ConstPageDef<FileTable> m_fileTable;
    FileDescriptor m_fileDescriptor;
    size_t m_highWaterMark;
};

//////////////////////////////////////////////////////////////////////////

class RawFileReader
{
public:
    RawFileReader(const std::shared_ptr<CacheManager>& cacheManager)
        : m_cacheManager(cacheManager)
        , m_curFilePos(0)
        , m_fileSize(0)
        , m_nextFileTable(PageIdx::INVALID)
    {}

    void open(FileDescriptor fileId)
    {
        assert(fileId != FileDescriptor());
        m_curFilePos = 0;
        m_fileSize = fileId.m_fileSize;
        ConstPageDef<FileTable> fileTable = m_cacheManager.loadPage<FileTable>(fileId.m_first);
        fileTable.m_page->insertInto(m_pageSequence);
        m_nextFileTable = fileTable.m_page->getNext();
    }

    Interval nextInterval(uint32_t maxSize)
    {
        if (m_pageSequence.empty())
        {
            if (m_nextFileTable == PageIdx::INVALID)
                return Interval(PageIdx::INVALID, PageIdx::INVALID);

            auto fileTable = m_cacheManager.loadPage<FileTable>(m_nextFileTable);
            fileTable.m_page->insertInto(m_pageSequence);
            m_nextFileTable = fileTable.m_page->getNext();
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
                begin = m_cacheManager.getRawFileInterface()->readPage(pageId, pageOffset, begin,
                                                                       begin + (4096 - pageOffset));
                nextInterval(1); // remove that page
            }
            else
            {
                begin = m_cacheManager.getRawFileInterface()->readPage(pageId, pageOffset, begin, begin + blockSize);
            }
        }

        // read full pages
        size_t pages = (end - begin) / 4096;
        while (pages > 0)
        {
            Interval iv = nextInterval((uint32_t) pages);
            begin = m_cacheManager.getRawFileInterface()->readPages(iv, begin);
            pages -= iv.length();
        }

        // partially read the next page
        if (end - begin)
        {
            PageIndex pageId = nextInterval(0).begin();
            begin = m_cacheManager.getRawFileInterface()->readPage(pageId, 0, begin, end);
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
    TypedCacheManager m_cacheManager;

    uint64_t m_curFilePos;
    uint64_t m_fileSize;
    uint32_t m_nextFileTable;
};

}

#endif
