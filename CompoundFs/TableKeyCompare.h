
#pragma once

#include "ByteString.h"

namespace TxFs
{

class TableKeyCompare
{

public:
    constexpr TableKeyCompare(const uint8_t* data) noexcept
        : m_data(data)
    {}

    bool operator()(uint16_t left, uint16_t right) const noexcept
    {
        auto l = ByteStringView::fromStream(m_data + left);
        auto r = ByteStringView::fromStream(m_data + right);
        return l < r;
    }

    bool operator()(uint16_t left, ByteStringView right) const noexcept
    {
        auto l = ByteStringView::fromStream(m_data + left);
        return l < right;
    }

    bool operator()(ByteStringView left, uint16_t right) const noexcept
    {
        auto r = ByteStringView::fromStream(m_data + right);
        return left < r;
    }

private:
    const uint8_t* m_data;
};


}



