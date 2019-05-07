#pragma once

#include <random>

namespace TxFs
{
    using PageIndex = uint32_t;

    class LogPage
    {
        uint64_t m_signature[2];
        uint32_t m_size;
        uint32_t m_data[1019];

    public:
        LogPage(PageIndex pageIndex)
            : m_size(0)
        {
            std::mt19937_64 mt(pageIndex);
            m_signature[0] = mt();
            m_signature[1] = mt();
        }

        bool checkSignature(PageIndex pageIndex) const
        {
            std::mt19937_64 mt(pageIndex);
            uint64_t sig[2] = { mt(), mt() };
            return sig[0] == m_signature[0] && sig[1] == m_signature[1];
        }


    };

    static_assert(sizeof(LogPage) == 4096);
}