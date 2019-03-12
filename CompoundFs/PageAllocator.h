

#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace TxFs
{
class PageAllocator
{
public:
    PageAllocator(size_t pagesPerBlock);

    std::shared_ptr<uint8_t> allocate();
    std::pair<size_t, size_t> trim();

private:
    std::shared_ptr<uint8_t> allocBlock();
    std::shared_ptr<uint8_t> makePage(std::shared_ptr<uint8_t> block, uint8_t* page);
    void free(std::shared_ptr<uint8_t> block, uint8_t* page);

private:
    size_t m_blocksAllocated;
    size_t m_pagesPerBlock;
    std::shared_ptr<uint8_t> m_block;
    uint8_t* m_currentPosInBlock;
    using BlockPage = std::pair<std::shared_ptr<uint8_t>, uint8_t*>;
    std::vector<BlockPage> m_freePages;
};

}
