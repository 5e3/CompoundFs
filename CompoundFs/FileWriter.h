#pragma once
#ifndef FILEWRITER_H
#define FILEWRITER_H

#include "Node.h"
#include "FileDescriptor.h"
#include "FileTable.h"
#include "TypedCacheManager.h"
#include "PageDef.h"
#include "FileInterface.h"
#include <algorithm>

namespace TxFs
{

class FileWriter final
{
public:
    FileWriter(const std::shared_ptr<CacheManager>& cacheManager, size_t highWaterMark = 250000)
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
        if (fileId != FileDescriptor())
        {
            m_fileDescriptor = fileId;
            m_fileTable = m_cacheManager.loadPage<FileTable>(fileId.m_last);
            m_fileTable.m_page->insertInto(m_pageSequence);
        }
        else
            createNew();
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
            m_cacheManager.getFileInterface()->writePage(m_pageSequence.back().end() - 1, pageOffset, begin,
                                                            newEndInPage);
            begin = newEndInPage;
        }

        // write full pages
        size_t pages = (end - begin) / 4096;
        while (pages > 0)
        {
            Interval iv = m_cacheManager.allocatePageInterval(pages);
            m_pageSequence.pushBack(iv);
            m_cacheManager.getFileInterface()->writePages(iv, begin);
            begin += static_cast<size_t>(iv.length()) * 4096;
            pages -= iv.length();
        }

        // write remaining bytes to partially filled page
        if (end - begin)
        {
            Interval iv = m_cacheManager.allocatePageInterval(1);
            m_pageSequence.pushBack(iv);
            m_cacheManager.getFileInterface()->writePage(iv.begin(), 0, begin, end);
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
        write((uint8_t*) &begin[0], (uint8_t*) &begin[0] + (end - begin));
    }

    uint64_t size() const { return m_fileDescriptor.m_fileSize; }

private:
    IntervalSequence m_pageSequence;
    TypedCacheManager m_cacheManager;
    ConstPageDef<FileTable> m_fileTable;
    FileDescriptor m_fileDescriptor;
    size_t m_highWaterMark;
};

//////////////////////////////////////////////////////////////////////////


}

#endif // FILEWRITER_H
