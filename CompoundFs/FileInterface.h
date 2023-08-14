
#pragma once

#include "Interval.h"
#include <stddef.h>



namespace TxFs
{
class Lock;
class CommitLock;

///////////////////////////////////////////////////////////////////////////////
/// Create=>create a new file overwriting an already existing, Open=>open an 
/// existing file or-else create a new file, ReadOnly=>open an existing file for
/// reading.
enum class OpenMode { Create, Open, ReadOnly };

///////////////////////////////////////////////////////////////////////////////
/// This is the abstraction of a file for CompoundFs. You have to allocate with newInterval()
/// before you write to the file. Locking is advisory. Make sure you have aquired the 
/// correct locks before you write to a file (linux will not even fail writes). All APIs 
/// will throw exceptions on failures.

class FileInterface
{
public:
    virtual ~FileInterface() = default;

    virtual Interval newInterval(size_t maxPages) = 0;
    virtual const uint8_t* writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end) = 0;
    virtual const uint8_t* writePages(Interval iv, const uint8_t* page) = 0;
    virtual uint8_t* readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const = 0;
    virtual uint8_t* readPages(Interval iv, uint8_t* page) const = 0;
    virtual size_t fileSizeInPages() const = 0; 
    virtual void flushFile() = 0;
    virtual void truncate(size_t numberOfPages) = 0;

    virtual Lock defaultAccess() = 0;
    virtual Lock readAccess() = 0;
    virtual Lock writeAccess() = 0;
    virtual CommitLock commitAccess(Lock&& writeLock) = 0;
};



}
