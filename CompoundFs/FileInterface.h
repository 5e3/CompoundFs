#pragma once

#include "Interval.h"
#include <vector>

namespace TxFs
{
class Lock;
class CommitLock;

class FileInterface
{
public:
    virtual ~FileInterface() = default;

    virtual Interval newInterval(size_t maxPages) = 0;
    virtual const uint8_t* writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end) = 0;
    virtual const uint8_t* writePages(Interval iv, const uint8_t* page) = 0;
    virtual uint8_t* readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const = 0;
    virtual uint8_t* readPages(Interval iv, uint8_t* page) const = 0;
    virtual size_t currentSize() const = 0; // file size in number of pages
    virtual void flushFile() = 0;
    virtual void truncate(size_t numberOfPages) = 0;

    virtual Lock defaultAccess() = 0;
    virtual Lock readAccess() = 0;
    virtual Lock writeAccess() = 0;
    virtual CommitLock commitAccess(Lock&& writeLock) = 0;
};

///////////////////////////////////////////////////////////////////////////////

inline void readPage(const FileInterface* fi, PageIndex idx, void* page)
{
    uint8_t* buffer = static_cast<uint8_t*>(page);
    fi->readPage(idx, 0, buffer, buffer + 4096);
}
inline void writePage(FileInterface* fi, PageIndex idx, const void* page)
{
    const uint8_t* buffer = static_cast<const uint8_t*>(page);
    fi->writePage(idx, 0, buffer, buffer + 4096);
}

inline void copyPage(FileInterface* fi, PageIndex from, PageIndex to)
{
    uint8_t buffer[4096];
    readPage(fi, from, buffer);
    writePage(fi, to, buffer);
}

inline bool isEqualPage(const FileInterface* fi, PageIndex p1, PageIndex p2)
{
    std::vector<uint8_t[4096]> buffer(2);
    readPage(fi, p1, buffer[0]);
    readPage(fi, p2, buffer[1]);
    return memcmp(buffer[0], buffer[1], 4096) == 0;
}

template<typename TCont>
inline void clearPages(FileInterface* fi, const TCont& cont)
{
    uint8_t buf[4096];
    memset(buf, 0, sizeof(buf));
    for (auto idx: cont)
        writePage(fi, idx, buf);
}

}
