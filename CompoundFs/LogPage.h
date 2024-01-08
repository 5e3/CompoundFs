#pragma once

#include <random>
#include <vector>
#include <algorithm>

namespace TxFs
{
using PageIndex = uint32_t;

class LogPage final
{
public:
    struct PageCopies
    {
        uint32_t m_original;
        uint32_t m_copy;
    };

    constexpr static size_t MAX_ENTRIES = 509;

private:
    uint32_t m_signature[4];
    uint32_t m_size;
    PageCopies m_pageCopies[MAX_ENTRIES];

public:
    uint32_t m_checkSum;

public:
    explicit LogPage() noexcept = default;

    LogPage(PageIndex pageIndex) noexcept
        : m_size(0)
    {
        std::minstd_rand mt(pageIndex);
        m_signature[0] = (uint32_t) mt();
        m_signature[1] = (uint32_t) mt();
        m_signature[2] = (uint32_t) mt();
        m_signature[3] = (uint32_t) mt();
    }

    bool checkSignature(PageIndex pageIndex) const noexcept
    {
        std::minstd_rand mt(pageIndex);
        uint32_t sig[4] = { (uint32_t) mt(), (uint32_t) mt(), (uint32_t) mt(), (uint32_t) mt() };
        return std::equal(sig, sig + 4, m_signature, m_signature + 4) && m_size <= MAX_ENTRIES;
    }

    std::vector<PageCopies> getPageCopies() const
    {
        return std::vector<PageCopies>(&m_pageCopies[0], &m_pageCopies[m_size]);
    }

    constexpr const PageCopies* begin() const noexcept { return m_pageCopies; }
    constexpr const PageCopies* end() const noexcept { return m_pageCopies + m_size; }

    constexpr bool pushBack(PageCopies pc) noexcept
    {
        if (m_size < MAX_ENTRIES)
        {
            m_pageCopies[m_size++] = pc;
            return true;
        }
        return false;
    }

    template <typename TIter>
    constexpr TIter pushBack(TIter begin, TIter end) noexcept
    {
        auto size = std::min(size_t(std::distance(begin, end)), MAX_ENTRIES - m_size);
        std::transform(begin, begin + size, m_pageCopies + m_size, [](auto value) {
            auto [copy, original] = value;
            return PageCopies { copy, original };
        });
        m_size += static_cast<uint32_t>(size);
        return begin + size;
    }

    constexpr size_t size() const noexcept { return m_size; }
};

constexpr bool operator==(LogPage::PageCopies lhs, LogPage::PageCopies rhs) noexcept
{
    return lhs.m_copy == rhs.m_copy && lhs.m_original == rhs.m_original;
}

static_assert(sizeof(LogPage) == 4096);
}
