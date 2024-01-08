

#pragma once

#include <type_traits>
#include <iterator>
#include <memory>
#include <algorithm>
#include <ranges>
#include "Blob.h"

namespace Rfx
{
template<typename T>
concept BitCompatible = (std::is_arithmetic_v<T> || std::is_enum_v<T>);

template <BitCompatible T, std::random_access_iterator TIter>
TIter copyBits(T value, TIter pos) requires std::same_as<std::iter_value_t<TIter>,std::byte>
{
    const std::byte* ptr = reinterpret_cast <const std::byte*>(&value);
    return std::copy(ptr, ptr + sizeof(T), pos);
}

template <std::forward_iterator TRange, std::random_access_iterator TIter>
TIter copyBits(TRange begin, TRange end, TIter pos)
    requires(BitCompatible<std::iter_value_t<TRange>> && std::same_as<std::iter_value_t<TIter>, std::byte>
             && !std::contiguous_iterator<TRange>)
{
    for (; begin != end; ++begin)
        pos = copyBits(*begin, pos);
    return pos;
}

template <std::contiguous_iterator TRange, std::random_access_iterator TIter>
TIter copyBits(TRange first, TRange last, TIter pos)
    requires(BitCompatible<std::iter_value_t<TRange>> && std::same_as<std::iter_value_t<TIter>, std::byte>)
{
    size_t size = last - first;
    const std::byte* ptr = reinterpret_cast<const std::byte*>(&(*first));
    return std::copy(ptr, ptr + (sizeof(std::iter_value_t<TRange>) * size), pos);
}

//////////////////////////////////////////////////////////////////////////////////////////

template <std::random_access_iterator TIter, BitCompatible T>
TIter copyBits(TIter pos, T& value)
    requires std::same_as<std::iter_value_t<TIter>, std::byte>
{
    std::byte* ptr = reinterpret_cast<std::byte*>(&value);
    std::copy(pos, pos + sizeof(T), ptr);
    return pos + sizeof(T);
}

template <std::random_access_iterator TIter, std::forward_iterator TRange>
TIter copyBits(TIter pos, TRange first, TRange last)
    requires(BitCompatible<std::iter_value_t<TRange>> && std::same_as<std::iter_value_t<TIter>, std::byte>
             && !std::contiguous_iterator<TRange>)
{
    for (; first != last; ++first)
        pos = copyBits(pos, *first);
    return pos;
}

template <std::random_access_iterator TIter, std::contiguous_iterator TRange>
TIter copyBits(TIter pos, TRange first, TRange last)
    requires(BitCompatible<std::iter_value_t<TRange>> && std::same_as<std::iter_value_t<TIter>, std::byte>)
{
    size_t size = last - first;
    std::byte* ptr = reinterpret_cast<std::byte*>(&(*first));
    auto endPos = pos + (sizeof(std::iter_value_t<TRange>) * size);
    std::copy(pos, endPos, ptr);
    return endPos;
}

template <std::ranges::sized_range TRange>
void pushBits(const TRange& range, Blob& blob)
    requires BitCompatible <std::ranges::range_value_t<TRange>>
{
    auto size = std::ranges::size(range);
    auto it = blob.grow(size * sizeof(std::ranges::range_value_t<TRange>));
    copyBits(range.begin(), range.end(), it);
}

template <std::ranges::range TRange>
void pushBits(const TRange& range, Blob& blob)
    requires(BitCompatible<std::ranges::range_value_t<TRange>> && !std::ranges::sized_range<TRange>)
{
    for (auto it = range.begin(); it != range.end(); ++it)
    {
        auto pos = blob.grow(sizeof(std::ranges::range_value_t<TRange>));
        copyBits(*it, pos);
    }
}

}