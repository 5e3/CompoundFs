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

    private:
        uint32_t m_signature[3];
        uint32_t m_size;
        PageCopies m_pageCopies[510];

    public:
        LogPage(PageIndex pageIndex)
            : m_size(0)
        {
            std::minstd_rand mt(pageIndex);
            m_signature[0] = mt();
            m_signature[1] = mt();
            m_signature[2] = mt();
        }

        bool checkSignature(PageIndex pageIndex) const
        {
            std::minstd_rand mt(pageIndex);
            uint32_t sig[3] = { mt(), mt(), mt()};
            return std::equal(sig, sig + 3, m_signature, m_signature + 3);
        }

        std::vector<PageCopies> getPageCopies() const
        {
            return std::vector<PageCopies>(&m_pageCopies[0], &m_pageCopies[m_size]);
        }

        bool pushBack(PageCopies pc)
        {
            m_pageCopies[m_size++] = pc;
            return m_size <= 510;
        }


    };

    constexpr bool operator ==(LogPage::PageCopies lhs, LogPage::PageCopies rhs) noexcept
    {
        return lhs.m_copy == rhs.m_copy && lhs.m_original == rhs.m_original;
    }


    static_assert(sizeof(LogPage) == 4096);
}