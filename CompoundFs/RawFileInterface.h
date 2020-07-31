#pragma once

#include "Interval.h"

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
    virtual void flushFile() = 0;
};

inline void copyPage(RawFileInterface* rfi, PageIndex from, PageIndex to)
{
    uint8_t buffer[4096];
    rfi->readPage(from, 0, buffer, buffer+sizeof(buffer));
    rfi->writePage(to, 0, buffer, buffer + sizeof(buffer));
}

}
