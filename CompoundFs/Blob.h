

#pragma once
#ifndef BLOB_H
#define BLOB_H

#include <cstdint>
#include <vector>
#include <algorithm>
#include <assert.h>

namespace TxFs
{

class BlobRef
{
    inline static uint8_t g_default = 0;

public:
    constexpr BlobRef() noexcept
        : m_data(&g_default)
    {}

    constexpr BlobRef(const uint8_t* val) noexcept
        : m_data((uint8_t*) val)
    {}

    constexpr uint16_t size() const noexcept { return *m_data + 1; }
    constexpr const uint8_t* begin() const noexcept { return m_data; }
    constexpr const uint8_t* end() const noexcept { return begin() + size(); }
    bool operator==(const BlobRef& rhs) const noexcept { return std::equal(begin(), end(), rhs.begin()); }
    bool operator!=(const BlobRef& rhs) const noexcept { return !(*this == rhs); }
    bool operator<(const BlobRef& rhs) const noexcept
    {
        return std::lexicographical_compare(begin() + 1, end(), rhs.begin() + 1, rhs.end());
    }
    bool isPrefix(const BlobRef& rhs) const noexcept
    {
        return std::mismatch(begin() + 1, end(), rhs.begin() + 1).first == end();
    }

    constexpr uint8_t* begin() noexcept { return m_data; }
    constexpr uint8_t* end() noexcept { return begin() + size(); }

protected:
    uint8_t* m_data;
};

//////////////////////////////////////////////////////////////////////////

class Blob : public BlobRef
{
public:
    Blob() noexcept = default;

    Blob(const char* str)
        : m_container(strlen(str) + 1)
    {
        assert(m_container.size() <= UINT8_MAX);
        m_data = &m_container[0];
        m_data[0] = uint8_t(m_container.size() - 1);
        std::copy(str, str + size() - 1, begin() + 1);
    }

    Blob(const uint8_t* val)
        : m_container(*val + size_t(1))
    {
        m_data = &m_container[0];
        std::copy(val, val + m_container.size(), m_data);
    }

    Blob(const BlobRef& br)
        : Blob(br.begin())
    {}

    Blob(const Blob& val)
        : m_container(val.m_container)
    {
        m_data = &m_container[0];
    }

    Blob(Blob&& val) noexcept
        : m_container(std::move(val.m_container))
    {
        m_data = &m_container[0];
    }

    Blob& operator=(const Blob& val)
    {
        auto tmp = val;
        swap(tmp);
        return *this;
    }

    Blob& operator=(const BlobRef& ref)
    {
        Blob val(ref);
        swap(val);
        return *this;
    }

    Blob& operator=(Blob&& val) noexcept
    {
        m_container = std::move(val.m_container);
        m_data = &m_container[0];
        return *this;
    }

    void swap(Blob& val) noexcept
    {
        std::swap(m_container, val.m_container);
        std::swap(m_data, val.m_data);
    }

private:
    std::vector<uint8_t> m_container;
};

//////////////////////////////////////////////////////////////////////////

class FixedBlob : public BlobRef
{
    uint8_t m_buffer[UINT8_MAX + 1];
    uint8_t* m_iterator;

public:
    FixedBlob() noexcept
        : m_iterator(m_buffer + 1)
    {
        m_buffer[0] = 0;
        m_data = m_buffer;
    }

    void pushBack(const uint8_t* valBegin, const uint8_t* valEnd)
    {
        assert(valEnd >= valBegin);
        uint8_t* endIterator = m_iterator + (valEnd - valBegin);
        if (endIterator > (m_buffer + sizeof(m_buffer)))
            throw std::runtime_error("Data exceeds max Blob size");
        m_iterator = std::copy(valBegin, valEnd, m_iterator);
        m_data[0] = uint8_t(m_iterator - begin() - 1);
    }

    template <typename T>
    constexpr void pushBack(const T& value)
    {
        auto begin = (uint8_t*) &value;
        auto end = begin + sizeof(T);
        pushBack(begin, end);
    }

    void pushBack(std::string_view str)
    {
        auto begin = (uint8_t*) &str[0];
        auto end = begin + str.size();
        pushBack(begin, end);
    }
};

//////////////////////////////////////////////////////////////////////////

class KeyCmp
{

public:
    constexpr KeyCmp(const uint8_t* data) noexcept
        : m_data(data)
    {}

    bool operator()(uint16_t left, uint16_t right) const noexcept
    {
        BlobRef l(m_data + left);
        BlobRef r(m_data + right);
        return l < r;
    }

    bool operator()(uint16_t left, const BlobRef& right) const noexcept
    {
        BlobRef l(m_data + left);
        return l < right;
    }

    bool operator()(const BlobRef& left, uint16_t right) const noexcept
    {
        BlobRef r(m_data + right);
        return left < r;
    }

private:
    const uint8_t* m_data;
};
}

#endif
