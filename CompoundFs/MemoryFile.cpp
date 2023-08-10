

#include "MemoryFile.h"
#include <memory>

using namespace TxFs;

MemoryFile::MemoryFile()
    : m_allocator(1024)
    , m_lockProtocol(std::make_unique<MemLockProtocol>())
{
    m_file.reserve(1024);
}

Interval MemoryFile::newInterval(size_t maxPages)
{
    PageIndex idx = (PageIndex) m_file.size();
    for (size_t i = 0; i < maxPages; i++)
        m_file.emplace_back(m_allocator.allocate());
    return Interval(idx, idx + uint32_t(maxPages));
}

const uint8_t* MemoryFile::writePage(PageIndex idx, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    auto p = m_file.at(idx);
    if (pageOffset + (end - begin) > 4096)
        throw std::runtime_error("MemoryFile::writePage over page boundary");
    std::copy(begin, end, p.get() + pageOffset);
    return end;
}

const uint8_t* MemoryFile::writePages(Interval iv, const uint8_t* page)
{
    for (auto idx = iv.begin(); idx < iv.end(); idx++)
    {
        auto p = m_file.at(idx);
        std::copy(page, page + 4096, p.get());
        page += 4096;
    }
    return page;
}

uint8_t* MemoryFile::readPage(PageIndex idx, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    auto p = m_file.at(idx);
    if (pageOffset + (end - begin) > 4096)
        throw std::runtime_error("MemoryFile::readPage over page boundary");
    return std::copy(p.get() + pageOffset, p.get() + pageOffset + (end - begin), begin);
}

uint8_t* MemoryFile::readPages(Interval iv, uint8_t* page) const
{
    for (auto idx = iv.begin(); idx < iv.end(); idx++)
    {
        auto p = m_file.at(idx);
        page = std::copy(p.get(), p.get() + 4096, page);
    }
    return page;
}

void MemoryFile::flushFile()
{}

void MemoryFile::truncate(size_t numberOfPages)
{
    assert(numberOfPages <= fileSizeInPages());
    m_file.resize(numberOfPages);
}

size_t MemoryFile::fileSizeInPages() const
{
    return m_file.size();
}

Lock MemoryFile::defaultAccess()
{
    return writeAccess();
}

Lock MemoryFile::readAccess()
{
    return m_lockProtocol->readAccess();
}

Lock MemoryFile::writeAccess()
{
    return m_lockProtocol->writeAccess();
}

CommitLock MemoryFile::commitAccess(Lock&& writeLock)
{
    return m_lockProtocol->commitAccess(std::move(writeLock));
}
