

#pragma once

#include <concepts>
#include <iterator>

namespace Rfx
{

template <std::integral Integer>
class CompressedInteger;

template<> class CompressedInteger<size_t>
{
public:
    size_t m_value;

    template<std::input_iterator It>
    It fromBytes(It begin) requires std::same_as<std::byte, std::iter_value_t<It>>
    {
        const byte mask = 0b01111111;
        size_t value = { *begin & mask};
//        while ()

    }


};


}