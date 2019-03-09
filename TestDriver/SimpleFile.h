
#pragma once

#include "../CompoundFs/CacheManager.h"
#include "../CompoundFs/PageAllocator.h"
#include <memory>

namespace TxFs
{

struct SimpleFile : RawFileInterface
{
    SimpleFile()
        : m_allocator(512)
    {}

    virtual Interval newInterval(size_t maxPages) override
    {
        PageIndex idx = (PageIndex) m_file.size();
        m_file.reserve(m_file.size() + maxPages);
        for (size_t i = 0; i < maxPages; i++)
            m_file.push_back(m_allocator.allocate());
        return Interval(idx, idx + uint32_t(maxPages));
    }

    virtual const uint8_t* writePage(PageIndex idx, size_t pageOffset, const uint8_t* page) override
    {
        auto p = m_file.at(idx);
        std::copy(page + pageOffset, page + 4096, p.get());
        return page + 4096 - pageOffset;
    }

    virtual const uint8_t* writePages(Interval iv, const uint8_t* page) override
    {
        for (auto idx = iv.begin(); idx < iv.end(); idx++)
        {
            auto p = m_file.at(idx);
            std::copy(page, page + 4096, p.get());
            page += 4096;
        }
        return page;
    }

    virtual uint8_t* readPage(PageIndex idx, size_t pageOffset, uint8_t* page) const override
    {
        auto p = m_file.at(idx);
        return std::copy(p.get() + pageOffset, p.get() + 4096, page);
    }

    virtual uint8_t* readPages(Interval iv, uint8_t* page) const
    {
        for (auto idx = iv.begin(); idx < iv.end(); idx++)
        {
            auto p = m_file.at(idx);
            page = std::copy(p.get(), p.get() + 4096, page);
        }
        return page;
    }

    std::vector<std::shared_ptr<uint8_t>> m_file;
    PageAllocator m_allocator;
};

}
