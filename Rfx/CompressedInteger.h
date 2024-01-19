

#pragma once

#include <concepts>
#include <iterator>
#include <bit>
#include <limits>

namespace Rfx
{

template <typename T>
struct ForEachMember;

constexpr int compressedSize(size_t value)
{
    value |= 0x1; // fix the 0 case
    return (std::bit_width(value) + 6) / 7;
}


template <std::integral Integer>
struct CompressedInteger;

template <>
struct CompressedInteger<size_t>
{
    size_t m_value;
};

template<>
struct ForEachMember<CompressedInteger<size_t>>
{

    template <typename TStream>
    static void write(CompressedInteger<size_t> value, TStream& stream)
    {
        const std::byte mask { 0b01111111 };
        const std::byte more = ~mask;
        const int size = compressedSize(value.m_value);
        for (int i = 0; i < size - 1; i++)
        {
            stream.write((std::byte(value.m_value) & mask ) | more);
            value.m_value >>= 7;
        }

        stream.write(std::byte(value.m_value) & mask);
    }

    template <typename TStream>
    static void read(CompressedInteger<size_t>& value, TStream& stream)
    {
        const std::byte mask { 0b01111111 };
        const std::byte more = ~mask;
        std::byte b;
        stream.read(b);
        value.m_value = size_t(b & mask);
        int count = 1;
        while ((b & more) != std::byte{0})
        {
            if (count == compressedSize(std::numeric_limits<size_t>::max()))
                throw std::runtime_error("read CompressedInteger out of bounds");
            stream.read(b);
            value.m_value |= size_t(b & mask) << (7 * count);
            count++;
        }
    }
};


}