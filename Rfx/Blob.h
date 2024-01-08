

#pragma once

#include <memory>
#include <cstddef>
#include <algorithm>
#include <bit>
#include <span>
#include <iterator>
#include <ranges>
#include <cassert>

namespace Rfx
{

class Blob
{
    std::unique_ptr<std::byte[]> m_storage;
    size_t m_size = 0;
    size_t m_capacity = 0;

public:
    using iterator = std::byte*;
    using const_iterator = const std::byte*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() noexcept { return m_storage.get(); }
    iterator end() noexcept { return m_storage.get() + m_size; }
    const_iterator begin() const noexcept { return m_storage.get(); }
    const_iterator end() const noexcept { return m_storage.get() + m_size; }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }
    reverse_iterator rbegin() noexcept { return std::make_reverse_iterator(end()); }
    reverse_iterator rend() noexcept { return std::make_reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const noexcept { return std::make_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return std::make_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    const_reverse_iterator crend() const noexcept { return rend(); }



public:
    Blob() = default;
    explicit Blob(size_t size);
    template <std::input_iterator It> Blob(It begin, It end) requires std::is_same_v<std::iter_value_t<It>, std::byte>;
    template <std::convertible_to<std::string_view> TString> Blob(TString&& str);
    Blob(const Blob& other);
    Blob(Blob&& other) noexcept;
    ~Blob() = default;

    Blob& operator=(const Blob&);
    Blob& operator=(Blob&&) noexcept;

    auto operator<=>(const Blob& rhs) const noexcept;
    bool operator==(const Blob& rhs) const noexcept;

    size_t size() const noexcept{ return m_size; }
    size_t capacity() const noexcept{ return m_capacity; }
    bool empty() const noexcept { return m_size == 0; }

    void reserve(size_t capacity);
    void resize(size_t size);
    iterator grow(size_t size);
    void clear() noexcept { resize(0); }
    void push_back(std::byte val) { *grow(1) = val; }
};

inline Blob::Blob(size_t size)
    : Blob()
{
    grow(size);
}

template <std::input_iterator It>
inline Blob::Blob(It begin, It end)
    requires std::is_same_v<std::iter_value_t<It>, std::byte>
{
    for (; begin != end; ++begin)
        *grow(1) = *begin;
}

template <std::convertible_to<std::string_view> TString>
inline Blob::Blob(TString&& str)
{
    std::string_view sv(std::forward<TString>(str));
    for (auto ch: sv)
        push_back((std::byte) ch );

}

inline Blob::Blob(const Blob& other)
    : Blob(other.size())
{
    std::copy(other.begin(), other.end(), begin());
}

inline Blob::Blob(Blob&& other) noexcept
    : m_storage(std::move(other.m_storage))
    , m_size(other.m_size)
    , m_capacity(other.m_capacity)
{
    other.m_size = 0;
    other.m_capacity = 0;
}

inline Blob& Blob::operator= (const Blob& other)
{
    if (&other != this)
    {
        if (other.size() > m_size)
            grow(other.size() - m_size);

        std::copy(other.begin(), other.end(), begin());
    }
    return *this;
}

inline Blob& Blob::operator=(Blob&& other) noexcept
{
    if (&other != this)
    {
        m_storage = std::move(other.m_storage);
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        other.m_size = 0;
        other.m_capacity = 0;
    }
    return *this;
}

inline auto Blob::operator<=>(const Blob& rhs) const noexcept
{
    return std::lexicographical_compare_three_way(
        begin(), end(), rhs.begin(), rhs.end(),
        [](std::byte lhs, std::byte rhs) { return uint8_t(lhs) <=> uint8_t(rhs); });
}

inline bool Blob::operator==(const Blob& rhs) const noexcept
{
    return (*this <=> rhs) == 0;
}

inline Blob::iterator Blob::grow(size_t size)
{
    auto newSize = size + m_size; 
    auto start = m_size;
    reserve(newSize);
    m_size = newSize;
    return m_storage.get() + start;
}

inline void Blob::resize(size_t size)
{
    if (m_size < size)
        reserve(size);
    m_size = size;
}

inline void Blob::reserve(size_t capacity)
{
    if (m_capacity < capacity)
    {
        m_capacity = std::max(std::bit_ceil(capacity), 1024ULL);
        auto storage = std::move(m_storage);
        m_storage = std::make_unique_for_overwrite<std::byte[]>(m_capacity);
        std::copy(storage.get(), storage.get() + m_size, begin());
    }
}


}

