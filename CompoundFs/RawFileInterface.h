#pragma once

#include "Interval.h"
#include <vector>

namespace TxFs
{

class RawFileInterface
{
public:
    virtual ~RawFileInterface() = default;

    virtual Interval newInterval(size_t maxPages) = 0;
    virtual const uint8_t* writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end) = 0;
    virtual const uint8_t* writePages(Interval iv, const uint8_t* page) = 0;
    virtual uint8_t* readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const = 0;
    virtual uint8_t* readPages(Interval iv, uint8_t* page) const = 0;
    virtual size_t currentSize() const = 0; // file size in number of pages
    virtual void flushFile() = 0;
};

///////////////////////////////////////////////////////////////////////////////

inline void readPage(const RawFileInterface* rfi, PageIndex idx, void* page)
{
    uint8_t* buffer = static_cast<uint8_t*>(page);
    rfi->readPage(idx, 0, buffer, buffer + 4096);
}
inline void writePage(RawFileInterface* rfi, PageIndex idx, const void* page)
{
    const uint8_t* buffer = static_cast<const uint8_t*>(page);
    rfi->writePage(idx, 0, buffer, buffer + 4096);
}

inline void copyPage(RawFileInterface* rfi, PageIndex from, PageIndex to)
{
    uint8_t buffer[4096];
    readPage(rfi, from, buffer);
    writePage(rfi, to, buffer);
}

inline bool isEqualPage(const RawFileInterface* rfi, PageIndex p1, PageIndex p2)
{
    std::vector<uint8_t[4096]> buffer(2);
    readPage(rfi, p1, buffer[0]);
    readPage(rfi, p2, buffer[1]);
    return memcmp(buffer[0], buffer[1], 4096) == 0;
}

template<typename TCont>
inline void clearPages(RawFileInterface* rfi, const TCont& cont)
{
    uint8_t buf[4096];
    memset(buf, 0, sizeof(buf));
    for (auto idx: cont)
        writePage(rfi, idx, buf);
}

}
