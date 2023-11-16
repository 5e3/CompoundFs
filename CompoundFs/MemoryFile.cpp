

#include "MemoryFile.h"
#include <memory>

using namespace TxFs;

MemoryFileBase::MemoryFileBase()
    : m_allocator(1024)
{
    m_file.reserve(1024);
}

Interval MemoryFileBase::newInterval(size_t maxPages)
{
    PageIndex idx = (PageIndex) m_file.size();
    for (size_t i = 0; i < maxPages; i++)
        m_file.emplace_back(m_allocator.allocate());
    return Interval(idx, idx + uint32_t(maxPages));
}

const uint8_t* MemoryFileBase::writePage(PageIndex idx, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    auto p = m_file.at(idx);
    if (pageOffset + (end - begin) > 4096)
        throw std::runtime_error("MemoryFileBase::writePage over page boundary");
    std::copy(begin, end, p.get() + pageOffset);
    return end;
}

const uint8_t* MemoryFileBase::writePages(Interval iv, const uint8_t* page)
{
    for (auto idx = iv.begin(); idx < iv.end(); idx++)
    {
        auto p = m_file.at(idx);
        std::copy(page, page + 4096, p.get());
        page += 4096;
    }
    return page;
}

uint8_t* MemoryFileBase::readPage(PageIndex idx, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    auto p = m_file.at(idx);
    if (pageOffset + (end - begin) > 4096)
        throw std::runtime_error("MemoryFileBase::readPage over page boundary");
    return std::copy(p.get() + pageOffset, p.get() + pageOffset + (end - begin), begin);
}

uint8_t* MemoryFileBase::readPages(Interval iv, uint8_t* page) const
{
    for (auto idx = iv.begin(); idx < iv.end(); idx++)
    {
        auto p = m_file.at(idx);
        page = std::copy(p.get(), p.get() + 4096, page);
    }
    return page;
}

void MemoryFileBase::flushFile()
{}

void MemoryFileBase::truncate(size_t numberOfPages)
{
    assert(numberOfPages <= fileSizeInPages());
    m_file.resize(numberOfPages);
}

size_t MemoryFileBase::fileSizeInPages() const
{
    return m_file.size();
}

MemoryFile mf;
auto p = std::unique_ptr<MemoryFile>();