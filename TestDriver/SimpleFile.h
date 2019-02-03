
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

    virtual PageIndex newPage() override
    {
        PageIndex id = (PageIndex) m_file.size();
        m_file.push_back(m_allocator.allocate());
        return id;
    }

    virtual void writePage(PageIndex id, const uint8_t* page) override
    {
        auto p = m_file.at(id);
        std::copy(page, page + 4096, p.get());
    }

    virtual void readPage(PageIndex id, uint8_t* page) const override
    {
        auto p = m_file.at(id);
        std::copy(p.get(), p.get() + 4096, page);
    }

    std::vector<std::shared_ptr<uint8_t>> m_file;
    PageAllocator m_allocator;
};

}
