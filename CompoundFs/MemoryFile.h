
#pragma once

#include "RawFileInterface.h"
#include "PageAllocator.h"
#include "LockProtocol.h"

#include <vector>
#include <stdint.h>
#include <mutex>
#include <shared_mutex>

namespace TxFs
{

class MemoryFile : public RawFileInterface
{
public:
    MemoryFile();

    Interval newInterval(size_t maxPages) override;
    const uint8_t* writePage(PageIndex idx, size_t pageOffset, const uint8_t* begin, const uint8_t* end) override;
    const uint8_t* writePages(Interval iv, const uint8_t* page) override;
    uint8_t* readPage(PageIndex idx, size_t pageOffset, uint8_t* begin, uint8_t* end) const override;
    uint8_t* readPages(Interval iv, uint8_t* page) const override;
    void flushFile() override;
    size_t currentSize() const override;
    void truncate(size_t numberOfPages) override;


    Lock readAccess() override;
    Lock writeAccess() override;
    CommitLock commitAccess(Lock&& writeLock) override;

private:
    std::vector<std::shared_ptr<uint8_t>> m_file;
    PageAllocator m_allocator;
    LockProtocol<std::shared_mutex, std::mutex> m_lockProtocol;

};

}
