
#pragma once

#include "FileInterface.h"
#include "PageAllocator.h"
#include "LockProtocol.h"
#include "SharedLock.h"


#include <vector>
#include <stdint.h>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <memory>



namespace TxFs
{

////////////////////////////////////////////////////////////////////////////////
/// MemoryFileBase 

class MemoryFileBase : public FileInterface
{
protected:
    MemoryFileBase();

public:
    Interval newInterval(size_t maxPages) override;
    const uint8_t* writePage(PageIndex idx, size_t pageOffset, const uint8_t* begin, const uint8_t* end) override;
    const uint8_t* writePages(Interval iv, const uint8_t* page) override;
    uint8_t* readPage(PageIndex idx, size_t pageOffset, uint8_t* begin, uint8_t* end) const override;
    uint8_t* readPages(Interval iv, uint8_t* page) const override;
    void flushFile() override;
    size_t fileSizeInPages() const override;
    void truncate(size_t numberOfPages) override;

private:
    PageAllocator m_allocator;
    std::vector<std::shared_ptr<uint8_t>> m_file;
};

////////////////////////////////////////////////////////////////////////////////
/// LockedMemoryFile stores every file operation in memory. Useful for testing the software.
/// Could be interesting for scenarios where a file is e.g. transfered over the network.
/// Note that this file object is not copyable...

template<typename TSharedMutex, typename TMutex>
class LockedMemoryFile : public MemoryFileBase
{
public:
    using TLockProtocol = LockProtocol<TSharedMutex, TMutex>;

public:
    LockedMemoryFile()
        : m_lockProtocol(std::make_unique<TLockProtocol>())
    {
    }

    LockedMemoryFile(std::unique_ptr<TLockProtocol> lockProtocol)
        : m_lockProtocol(std::move(lockProtocol))
    {
    }

    Lock defaultAccess() override;
    Lock readAccess() override;
    Lock writeAccess() override;
    CommitLock commitAccess(Lock&& writeLock) override;

private:
    std::unique_ptr<TLockProtocol> m_lockProtocol;
};

////////////////////////////////////////////////////////////////////////////////

using MemoryFile = LockedMemoryFile<SharedLock,SharedLock>;
//using MemoryFile = LockedMemoryFile<std::shared_mutex, std::mutex>;

template <typename TSharedMutex, typename TMutex>
Lock LockedMemoryFile<TSharedMutex, TMutex>::defaultAccess()
{
    return LockedMemoryFile::writeAccess();
}

template <typename TSharedMutex, typename TMutex>
Lock LockedMemoryFile<TSharedMutex, TMutex>::readAccess()
{
    return m_lockProtocol->readAccess();
}

template <typename TSharedMutex, typename TMutex>
Lock LockedMemoryFile<TSharedMutex, TMutex>::writeAccess()
{
    return m_lockProtocol->writeAccess();
}

template <typename TSharedMutex, typename TMutex>
CommitLock LockedMemoryFile<TSharedMutex, TMutex>::commitAccess(Lock&& writeLock)
{
    return m_lockProtocol->commitAccess(std::move(writeLock));
}


}
