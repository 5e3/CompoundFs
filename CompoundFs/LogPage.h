#pragma once

#include <random>
#include <vector>
#include <algorithm>

namespace TxFs
{
using PageIndex = uint32_t;

class LogPage
{
public:
    struct PageCopies
    {
        uint32_t m_original;
        uint32_t m_copy;
    };

    constexpr static size_t MAX_ENTRIES = 510;

private:
    uint32_t m_signature[3];
    uint32_t m_size;
    PageCopies m_pageCopies[MAX_ENTRIES];

public:
    LogPage(PageIndex pageIndex) noexcept
        : m_size(0)
    {
        std::minstd_rand mt(pageIndex);
        m_signature[0] = mt();
        m_signature[1] = mt();
        m_signature[2] = mt();
    }

    bool checkSignature(PageIndex pageIndex) const noexcept
    {
        std::minstd_rand mt(pageIndex);
        uint32_t sig[3] = { mt(), mt(), mt() };
        return std::equal(sig, sig + 3, m_signature, m_signature + 3);
    }

    std::vector<PageCopies> getPageCopies() const
    {
        return std::vector<PageCopies>(&m_pageCopies[0], &m_pageCopies[m_size]);
    }

    constexpr bool pushBack(PageCopies pc) noexcept
    {
        if (m_size < MAX_ENTRIES)
        {
            m_pageCopies[m_size++] = pc;
            return true;
        }
        return false;
    }

    template <typename ITERATOR>
    constexpr bool pushBack(ITERATOR begin, ITERATOR end) noexcept
    {
        size_t size = std::distance(begin, end);
        if (m_size + size <= MAX_ENTRIES)
        {
            for (size_t i = m_size; begin != end; ++begin, i++)
                m_pageCopies[i] = *begin;
            m_size += static_cast<uint32_t>(size);
            return true;
        }
        return false;
    }

    constexpr size_t size() const noexcept { return m_size; }
};

constexpr bool operator==(LogPage::PageCopies lhs, LogPage::PageCopies rhs) noexcept
{
    return lhs.m_copy == rhs.m_copy && lhs.m_original == rhs.m_original;
}

static_assert(sizeof(LogPage) == 4096);
}
