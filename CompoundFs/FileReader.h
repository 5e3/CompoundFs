#pragma once

#include "Node.h"
#include "FileDescriptor.h"
#include "FileTable.h"
#include "TypedCacheManager.h"
#include "PageDef.h"
#include "FileInterface.h"
#include <algorithm>

namespace TxFs
{

//////////////////////////////////////////////////////////////////////////

class FileReader final
{
public:
    FileReader(const std::shared_ptr<CacheManager>& cacheManager)
        : m_cacheManager(cacheManager)
        , m_curFilePos(0)
        , m_fileSize(0)
        , m_nextFileTable(PageIdx::INVALID)
    {}

    void open(FileDescriptor fileId)
    {
        m_curFilePos = 0;
        m_fileSize = fileId.m_fileSize;
        if (fileId != FileDescriptor())
        {
            ConstPageDef<FileTable> fileTable = m_cacheManager.loadPage<FileTable>(fileId.m_first);
            fileTable.m_page->insertInto(m_pageSequence);
            m_nextFileTable = fileTable.m_page->getNext();
        }
        else
            m_nextFileTable = PageIdx::INVALID;
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
                begin = m_cacheManager.getFileInterface()->readPage(pageId, pageOffset, begin,
                                                                       begin + (4096 - pageOffset));
                nextInterval(1); // remove that page
            }
            else
            {
                begin = m_cacheManager.getFileInterface()->readPage(pageId, pageOffset, begin, begin + blockSize);
            }
        }

        // read full pages
        size_t pages = (end - begin) / 4096;
        while (pages > 0)
        {
            Interval iv = nextInterval((uint32_t) pages);
            begin = m_cacheManager.getFileInterface()->readPages(iv, begin);
            pages -= iv.length();
        }

        // partially read the next page
        if (end - begin)
        {
            PageIndex pageId = nextInterval(0).begin();
            begin = m_cacheManager.getFileInterface()->readPage(pageId, 0, begin, end);
        }

        m_curFilePos += blockSize;
        return begin;
    }

    uint64_t bytesLeft() const { return m_fileSize - m_curFilePos; }
    uint64_t size() const { return m_fileSize; }

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

