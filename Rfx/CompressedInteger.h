

#pragma once

#include <concepts>
#include <cstddef>
#include <bit>
#include <limits>
#include <stdexcept>

namespace Rfx
{

template <typename T>
struct StreamRule;

///////////////////////////////////////////////////////////////////////////////

constexpr int compressedSize(size_t value)
{
    value |= 0x1; // fix the 0 case
    return (std::bit_width(value) + 6) / 7;
}

///////////////////////////////////////////////////////////////////////////////

template <std::integral Integer>
struct CompressedInteger;

///////////////////////////////////////////////////////////////////////////////

 template <>
 struct CompressedInteger<size_t>
{
     size_t m_value;
     operator size_t() const { return m_value; }
 };

 ///////////////////////////////////////////////////////////////////////////////

 template <>
struct StreamRule<CompressedInteger<size_t>>
{
    static constexpr std::byte MASK { 0b01111111 }; // Mask for extracting 7 bits    
    static constexpr std::byte MORE { 0b10000000 }; // Indicator that more bytes are to follow

    template <typename TStream>
    static void write(CompressedInteger<size_t> value, TStream&& stream)
    {
        size_t temp = value.m_value;

        // Write all the bytes except the last one
        while (temp >= 0b10000000) // While more than 7 bits remain
        {
            stream.write(std::byte(temp) & MASK | MORE); // Write 7 bits with the continuation bit set
            temp >>= 7;                                  // Shift to get the next 7 bits
        }
        
        // Write the last byte without the continuation bit
        stream.write(std::byte(temp) & MASK);
    }

    template <typename TStream>
    static void read(CompressedInteger<size_t>& value, TStream&& stream)
    {
        size_t result = 0;
        int shift = 0;
        std::byte b;

        // Read bytes while the continuation bit is set
        do
        {
            if (shift >= compressedSize(std::numeric_limits<size_t>::max()) * 7)
                throw std::runtime_error("read CompressedInteger out of bounds");

            stream.read(b);
            result |= (size_t(b & MASK) << shift);      // Extract 7 bits and shift them into position
            shift += 7;                                 // Move to the next 7-bit group
        } while (std::to_integer<uint8_t>(b & MORE));   // Continue if the continuation bit is set

        value.m_value = result; // Set the result in the compressed integer
    }
};

}