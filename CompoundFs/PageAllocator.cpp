

#include "PageAllocator.h"
#include <algorithm>
#include <unordered_map>
#include <windows.h>

using namespace TxFs;

PageAllocator::PageAllocator(size_t pagesPerBlock)
    : m_blocksAllocated(0)
    , m_pagesPerBlock(max(pagesPerBlock, 16))
    , m_block(allocBlock())
    , m_currentPosInBlock(m_block.get())
{}

std::shared_ptr<uint8_t> PageAllocator::allocate()
{
    if (!m_freePages.empty())
    {
        auto page = m_freePages.back();
        m_freePages.pop_back();
        return makePage(page.first, page.second);
    }

    auto page = makePage(m_block, m_currentPosInBlock);
    m_currentPosInBlock += 4096;
    if ((m_block.get() + m_pagesPerBlock * 4096) != m_currentPosInBlock)
        return page;

    m_block = allocBlock();
    m_currentPosInBlock = m_block.get();
    return page;
}

std::pair<size_t, size_t> PageAllocator::trim()
{
    std::unordered_map<std::shared_ptr<uint8_t>, std::vector<uint8_t*>> blockToPage;
    blockToPage.reserve(m_blocksAllocated);
    for (auto& bp: m_freePages)
    {
        auto it = blockToPage.find(bp.first);
        if (it == blockToPage.end())
        {
            it = blockToPage.emplace(bp.first, std::vector<uint8_t*>()).first;
            it->second.reserve(m_pagesPerBlock);
        }
        it->second.push_back(bp.second);
    }

    m_freePages.clear();
    m_blocksAllocated -= blockToPage.size();
    for (auto it: blockToPage)
    {
        if (it.second.size() != m_pagesPerBlock)
        {
            for (auto p: it.second)
                m_freePages.emplace_back(it.first, p);
            m_blocksAllocated++;
        }
    }
    m_freePages.shrink_to_fit();
    blockToPage.clear();
    return std::make_pair(m_blocksAllocated, m_freePages.size());
}

std::shared_ptr<uint8_t> PageAllocator::allocBlock()
{
    uint8_t* block = (uint8_t*) ::VirtualAlloc(NULL, m_pagesPerBlock * 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (block == 0)
        throw std::bad_alloc();

    m_blocksAllocated++;
    return std::shared_ptr<uint8_t>(block, [](uint8_t* b) { ::VirtualFree(b, 0, MEM_RELEASE); });
}

void PageAllocator::free(std::shared_ptr<uint8_t> block, uint8_t* page)
{
    m_freePages.emplace_back(block, page);
}

std::shared_ptr<uint8_t> PageAllocator::makePage(std::shared_ptr<uint8_t> block, uint8_t* page)
{
    return std::shared_ptr<uint8_t>(page, [this, block](uint8_t* page) { this->free(block, page); });
}
