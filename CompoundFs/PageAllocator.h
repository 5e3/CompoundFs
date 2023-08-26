

#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace TxFs
{
//////////////////////////////////////////////////////////////////////////////////
/// PageAllocator is a bump-pointer allocator. It allocates pages from a 
/// block with the size of multiple pages. The pages are handed out via a 
/// std::shared_ptr<> which upon deletion returns the memory to the PageAllocator
/// instance where it is kept in container for re-usage. PageAllocator can be 
/// moved. PageAllocator::trim() will return as many blocks as possible to the 
/// system.

class PageAllocator final
{
public:
    PageAllocator(size_t pagesPerBlock=16);

    std::shared_ptr<uint8_t> allocate();
    std::pair<size_t, size_t> trim();

private:
    std::shared_ptr<uint8_t> allocBlock();
    std::shared_ptr<uint8_t> makePage(std::shared_ptr<uint8_t> block, uint8_t* page);

private:
    using BlockPage = std::pair<std::shared_ptr<uint8_t>, uint8_t*>;
    size_t m_blocksAllocated;
    size_t m_pagesPerBlock;
    std::unique_ptr<std::vector<BlockPage>> m_freePages; // to make lambdas immune to move ops
    std::shared_ptr<uint8_t> m_block;
    uint8_t* m_currentPosInBlock;
};

}
