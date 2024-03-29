
#include "PageAllocator.h"
#include <algorithm>
#include <unordered_map>
#include <system_error>

using namespace TxFs;

PageAllocator::PageAllocator(size_t pagesPerBlock)
    : m_blocksAllocated(0)
    , m_pagesPerBlock(std::max(pagesPerBlock, size_t(16)))
    , m_currentPosInBlock(nullptr)
{}

std::shared_ptr<uint8_t> PageAllocator::allocate()
{
    if (!m_block)
    {
        m_blocksAllocated = 0;
        m_freePages = std::make_unique<std::vector<BlockPage>>();
        m_block = allocBlock();
        m_currentPosInBlock = m_block.get();
    }
    if (!m_freePages->empty())
    {
        auto page = m_freePages->back();
        m_freePages->pop_back();
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
    if (!m_freePages)
        return std::pair<size_t, size_t>(0, 0);

    std::unordered_map<std::shared_ptr<uint8_t>, std::vector<uint8_t*>> blockToPage;
    blockToPage.reserve(m_blocksAllocated);
    for (const auto& bp: *m_freePages)
    {
        auto it = blockToPage.find(bp.first);
        if (it == blockToPage.end())
        {
            it = blockToPage.emplace(bp.first, std::vector<uint8_t*>()).first;
            it->second.reserve(m_pagesPerBlock);
        }
        it->second.push_back(bp.second);
    }

    m_freePages->clear();
    m_blocksAllocated -= blockToPage.size();
    for (const auto& it: blockToPage)
    {
        if (it.second.size() != m_pagesPerBlock)
        {
            for (auto p: it.second)
                m_freePages->emplace_back(it.first, p);
            m_blocksAllocated++;
        }
    }
    m_freePages->shrink_to_fit();
    blockToPage.clear();
    return std::make_pair(m_blocksAllocated, m_freePages->size());
}

#ifdef _WINDOWS

#define NOMINMAX 1
#include <windows.h>

std::shared_ptr<uint8_t> PageAllocator::allocBlock()
{
    // reserves the memory but allocates only on touch (when you start using it)
    uint8_t* block = (uint8_t*) ::VirtualAlloc(nullptr, m_pagesPerBlock * 4096, MEM_COMMIT, PAGE_READWRITE);

    // actually allocates the memory immediately 
    //uint8_t* block = (uint8_t*) ::VirtualAlloc(nullptr, m_pagesPerBlock * 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (block == nullptr)
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "PageAllocator");

    m_blocksAllocated++;
    return std::shared_ptr<uint8_t>(block, [](uint8_t* b) { ::VirtualFree(b, 0, MEM_RELEASE); });
}

#else

std::shared_ptr<uint8_t> PageAllocator::allocBlock()
{
    std::shared_ptr<uint8_t> block(new uint8_t[m_pagesPerBlock * 4096], [](uint8_t* b) { delete[] b; });
    m_blocksAllocated++;
    return block;
}

#endif

std::shared_ptr<uint8_t> PageAllocator::makePage(std::shared_ptr<uint8_t> block, uint8_t* page)
{
    // Note that m_freePage is a unique_ptr<> and the lambda uses the raw pointer to the vector<>
    // which makes PageAllocator movable. The raw pointer to the vector<> after a move operation 
    // is still the same.
    return std::shared_ptr<uint8_t>(page, [block,fp=m_freePages.get()](uint8_t* page) { fp->emplace_back(block, page); });
}
