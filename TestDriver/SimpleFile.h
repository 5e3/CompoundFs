
#pragma once

#include "../CompoundFs/RawFileInterface.h"
#include "../CompoundFs/PageAllocator.h"
#include <memory>

namespace TxFs
{

struct SimpleFile : RawFileInterface
{
    SimpleFile()
        : m_allocator(1024)
    {
        m_file.reserve(1024);
    }

    virtual Interval newInterval(size_t maxPages) override
    {
        PageIndex idx = (PageIndex) m_file.size();
        for (size_t i = 0; i < maxPages; i++)
            m_file.emplace_back(m_allocator.allocate());
        return Interval(idx, idx + uint32_t(maxPages));
    }

    virtual const uint8_t* writePage(PageIndex idx, size_t pageOffset, const uint8_t* begin,
                                     const uint8_t* end) override
    {
        auto p = m_file.at(idx);
        std::copy(begin, end, p.get() + pageOffset);
        return end;
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

    virtual uint8_t* readPage(PageIndex idx, size_t pageOffset, uint8_t* begin, uint8_t* end) const override
    {
        auto p = m_file.at(idx);
        return std::copy(p.get() + pageOffset, p.get() + pageOffset + (end - begin), begin);
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

    virtual void commit() override {}

    void clearPages(const std::vector<PageIndex>& pages)
    {
        for (auto page: pages)
            memset(m_file.at(page).get(), 0, 4096);
    }

    std::vector<std::shared_ptr<uint8_t>> m_file;
    PageAllocator m_allocator;
};

}
