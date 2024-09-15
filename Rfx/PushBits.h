

#pragma once

#include <type_traits>
#include <iterator>
#include <memory>
#include <algorithm>
#include <ranges>
#include <stdexcept>
#include "Blob.h"

namespace Rfx
{

#ifdef _DEBUG
constexpr bool debug_mode = true;
#else
constexpr bool debug_mode = false;
#endif

template<typename T>
concept BitStreamable = (std::is_arithmetic_v<T> || std::is_enum_v<T>);

template <BitStreamable T, std::random_access_iterator TIter>
TIter copyBits(T value, TIter pos) requires std::same_as<std::iter_value_t<TIter>,std::byte>
{
    const std::byte* ptr = reinterpret_cast <const std::byte*>(&value);
    return std::copy(ptr, ptr + sizeof(T), pos);
}

template <std::forward_iterator TRange, std::random_access_iterator TIter>
TIter copyBits(TRange begin, TRange end, TIter pos)
    requires(BitStreamable<std::iter_value_t<TRange>> && std::same_as<std::iter_value_t<TIter>, std::byte>
             && !std::contiguous_iterator<TRange>)
{
    for (; begin != end; ++begin)
        pos = copyBits(*begin, pos);
    return pos;
}

template <std::contiguous_iterator TRange, std::random_access_iterator TIter>
TIter copyBits(TRange first, TRange last, TIter pos)
    requires(BitStreamable<std::iter_value_t<TRange>> && std::same_as<std::iter_value_t<TIter>, std::byte>)
{
    size_t size = last - first;
    if (debug_mode && size == 0)
        return pos;

    const std::byte* ptr = reinterpret_cast<const std::byte*>(&(*first));
    return std::copy(ptr, ptr + (sizeof(std::iter_value_t<TRange>) * size), pos);
}

///////////////////////////////////////////////////////////////////////////////

template <std::random_access_iterator TIter, BitStreamable T>
TIter copyBits(TIter pos, T& value)
    requires std::same_as<std::iter_value_t<TIter>, std::byte>
{
    std::byte* ptr = reinterpret_cast<std::byte*>(&value);
    std::copy(pos, pos + sizeof(T), ptr);
    return pos + sizeof(T);
}

template <std::random_access_iterator TIter, std::forward_iterator TRange>
TIter copyBits(TIter pos, TRange first, TRange last)
    requires(BitStreamable<std::iter_value_t<TRange>> && std::same_as<std::iter_value_t<TIter>, std::byte>
             && !std::contiguous_iterator<TRange>)
{
    for (; first != last; ++first)
        pos = copyBits(pos, *first);
    return pos;
}

template <std::random_access_iterator TIter, std::contiguous_iterator TRange>
TIter copyBits(TIter pos, TRange first, TRange last)
    requires(BitStreamable<std::iter_value_t<TRange>> && std::same_as<std::iter_value_t<TIter>, std::byte>)
{
    size_t size = last - first;
    if (debug_mode && size == 0)
        return pos;

    std::byte* ptr = reinterpret_cast<std::byte*>(&(*first));
    auto endPos = pos + (sizeof(std::iter_value_t<TRange>) * size);
    std::copy(pos, endPos, ptr);
    return endPos;
}

///////////////////////////////////////////////////////////////////////////////

template <BitStreamable T>
void pushBits(T value, Blob& blob)
{
    auto it = blob.grow(sizeof(T));
    copyBits(value, it);
}

template <std::ranges::sized_range TRange>
void pushBits(const TRange& range, Blob& blob)
    requires BitStreamable <std::ranges::range_value_t<TRange>>
{
    auto size = std::ranges::size(range);
    auto it = blob.grow(size * sizeof(std::ranges::range_value_t<TRange>));
    copyBits(range.begin(), range.end(), it);
}


///////////////////////////////////////////////////////////////////////////////

using BlobRange = std::ranges::subrange<Blob::const_iterator>;

/// for single BitStreamables (like doubles)
template<BitStreamable T>
auto popBits(T& value, BlobRange blobRange)
{
    if (blobRange.size() < sizeof(T))
        throw std::out_of_range("popBits()");
    return copyBits(blobRange.begin(), value);
}

template <BitStreamable T>
T popBits(BlobRange& blobRange)
{
    T value {};
    auto it = popBits(value, blobRange);
    blobRange = BlobRange(it, blobRange.end());
    return value;
}

/// for containers of BitStreamable (like std::list<double>)
template <std::ranges::sized_range TRange>
auto popBits(TRange& valueRange, BlobRange blobRange)
    requires BitStreamable<std::ranges::range_value_t<TRange>>
{
    if (blobRange.size() < valueRange.size() * sizeof(std::ranges::range_value_t<TRange>))
        throw std::out_of_range("popBits()");
    return copyBits(blobRange.begin(), valueRange.begin(), valueRange.end());
}




}