
#pragma once

#include <stdint.h>
#include <assert.h>
#include <algorithm>
#include <string_view>
#include <type_traits>
#include <stdexcept>
#include <vector>

namespace TxFs
{
template <typename T>
using EnableWithString = std::enable_if_t<std::is_convertible_v<T, std::string_view>>;

class ByteStringView final
{
public:
    constexpr ByteStringView() noexcept;
    constexpr ByteStringView(const uint8_t* data, uint8_t length) noexcept;

    template <typename TStr, typename = EnableWithString<TStr>>
    ByteStringView(TStr&& str);

    template <typename TStr, typename = EnableWithString<TStr>>
    ByteStringView& operator=(TStr&& str);

    friend bool operator==(ByteStringView lhs, ByteStringView rhs);
    friend bool operator!=(ByteStringView lhs, ByteStringView rhs);
    friend bool operator<(ByteStringView lhs, ByteStringView rhs);

    constexpr size_t size() const noexcept;
    constexpr const uint8_t* data() const noexcept;
    constexpr const uint8_t* end() const noexcept; 
    static constexpr ByteStringView fromStream(const uint8_t* stream) noexcept;

private:
    const uint8_t* m_data;
    uint8_t m_length;
};

///////////////////////////////////////////////////////////////////////////////

class ByteString final
{
public:
    ByteString() noexcept = default;
    ByteString(ByteStringView bsv) noexcept;
    template <typename TStr, typename = EnableWithString<TStr>>
    ByteString(TStr&& str);

    ByteString& operator=(ByteStringView bsv) noexcept;
    template <typename TStr, typename = EnableWithString<TStr>>
    ByteString& operator=(TStr&& str);
    operator ByteStringView() const noexcept;
    size_t size() const noexcept; 

private:
    std::vector<uint8_t> m_buffer;
};


///////////////////////////////////////////////////////////////////////////////

template <typename T>
using EnableSimpleTypes = std::enable_if_t<std::is_trivially_copyable<T>::value>;

class ByteStringStream final
{
public:
    ByteStringStream() noexcept;

    bool isPrefix(ByteStringView rhs) const noexcept;
    operator ByteStringView() const noexcept;

    void push(ByteStringView bsv);
    template <typename T, typename = EnableSimpleTypes<T>>
    void push(const T& val) noexcept;
    template <typename T, typename = EnableSimpleTypes<T>>
    static ByteStringView pop(T& val, ByteStringView bsv);

private:
    uint8_t m_buffer[255];
    uint8_t* m_pos;
};


///////////////////////////////////////////////////////////////////////////////
constexpr ByteStringView::ByteStringView() noexcept
    : m_data(nullptr)
    , m_length(0) 
{}

constexpr ByteStringView::ByteStringView(const uint8_t* data, uint8_t length) noexcept
    : m_data(data)
    , m_length(length) 
{}

template <typename TStr, typename>
inline ByteStringView::ByteStringView(TStr&& str)
{
    std::string_view sv { str };
    if (sv.size() > std::numeric_limits<uint8_t>::max())
        throw std::runtime_error("ByteStringView: size too big");

    m_data = reinterpret_cast<const uint8_t*>(sv.data());
    m_length = static_cast<uint8_t>(sv.size());
}

template <typename TStr, typename>
inline ByteStringView& ByteStringView::operator=(TStr&& str)
{
    ByteStringView bsv = str;
    *this = bsv;
    return *this;
}

constexpr size_t ByteStringView::size() const noexcept
{
    return m_length;
}

constexpr const uint8_t* ByteStringView::data() const noexcept
{
    return m_data;
}

constexpr const uint8_t* ByteStringView::end() const noexcept
{
    return m_data + m_length;
}

constexpr ByteStringView ByteStringView::fromStream(const uint8_t* stream) noexcept
{
    return ByteStringView(stream + 1, *stream);
}

inline uint8_t* toStream(ByteStringView bsv, uint8_t* dest)
{
    *dest = static_cast<uint8_t>(bsv.size());
    return std::copy(bsv.data(), bsv.data() + bsv.size(), ++dest);
}

inline bool operator==(ByteStringView lhs, ByteStringView rhs)
{
    return std::equal(lhs.m_data, lhs.m_data + lhs.m_length, rhs.m_data, rhs.m_data + rhs.m_length);
}

inline bool operator!=(ByteStringView lhs, ByteStringView rhs)
{
    return !(lhs == rhs);
}

inline bool operator<(ByteStringView lhs, ByteStringView rhs)
{
    return std::lexicographical_compare(lhs.m_data, lhs.m_data + lhs.m_length, rhs.m_data, rhs.m_data + rhs.m_length);
}

///////////////////////////////////////////////////////////////////////////////

inline ByteString::ByteString(ByteStringView bsv) noexcept
    : m_buffer(bsv.data(), bsv.data() + bsv.size())
{}

inline ByteString& ByteString::operator=(ByteStringView bsv) noexcept
{
    ByteString bs(bsv);
    std::swap(bs.m_buffer, m_buffer);
    return *this;
}

template <typename TStr, typename>
inline ByteString::ByteString(TStr&& str)
    : ByteString(ByteStringView(str))
{}

template <typename TStr, typename>
inline ByteString& ByteString::operator=(TStr&& str)
{
    *this = ByteString(str);
    return *this;
}

inline ByteString::operator ByteStringView() const noexcept
{
    return ByteStringView(&m_buffer[0], static_cast<uint8_t>(m_buffer.size()));
}

inline size_t ByteString::size() const noexcept
{
    return m_buffer.size();
}

///////////////////////////////////////////////////////////////////////////////

inline ByteStringStream::ByteStringStream() noexcept
    : m_pos(m_buffer)
{}

inline bool ByteStringStream::isPrefix(ByteStringView rhs) const noexcept
{
    const auto self = static_cast<ByteStringView>(*this);
    return std::mismatch(self.data(), self.end(), rhs.data()).first == self.end();
}

inline ByteStringStream::operator ByteStringView() const noexcept
{
    return ByteStringView(m_buffer, static_cast<uint8_t>(m_pos - m_buffer));
}

inline void ByteStringStream::push(ByteStringView bsv)
{
    if ((m_buffer + sizeof(m_buffer)) < (m_pos + bsv.size()))
        throw std::runtime_error("ByteStringStream: overflow");

    m_pos = std::copy(bsv.data(), bsv.end(), m_pos);
}

template <typename T, typename>
inline void ByteStringStream::push(const T& val) noexcept
{
    static_assert(!std::is_same_v<T, std::string>);
    static_assert(!std::is_same_v<T, std::string_view>);

    auto begin = reinterpret_cast<const uint8_t*>(&val);
    auto end = begin + sizeof(T);
    assert((m_pos + sizeof(T)) < (m_buffer + sizeof(m_buffer)));

    m_pos = std::copy(begin, end, m_pos);
}

template <typename T, typename>
inline ByteStringView ByteStringStream::pop(T& val, ByteStringView bsv)
{
    auto beginDst = reinterpret_cast<uint8_t*>(&val);
    auto end = bsv.data() + sizeof(T);
    std::copy(bsv.data(), end, beginDst);
    return ByteStringView(end, static_cast<uint8_t> (bsv.size() - sizeof(T)));
}

}
