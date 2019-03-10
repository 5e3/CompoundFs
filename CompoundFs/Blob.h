

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
    BlobRef()
        : m_data(&g_default)
    {}

    BlobRef(const uint8_t* val)
        : m_data((uint8_t*) val)
    {}

    uint16_t size() const { return *m_data + 1; }
    const uint8_t* begin() const { return m_data; }
    const uint8_t* end() const { return begin() + size(); }
    bool operator==(const BlobRef& lhs) const { return std::equal(begin(), end(), lhs.begin()); }
    bool operator!=(const BlobRef& lhs) const { return !(*this == lhs); }
    bool operator<(const BlobRef& rhs) const
    {
        return std::lexicographical_compare(begin() + 1, end(), rhs.begin() + 1, rhs.end());
    }

    uint8_t* begin() { return m_data; }
    uint8_t* end() { return begin() + size(); }

protected:
    uint8_t* m_data;
};

//////////////////////////////////////////////////////////////////////////

class Blob : public BlobRef
{
public:
    Blob() = default;

    Blob(const char* str)
        : m_container(strlen(str) + 1)
    {
        m_data = &m_container[0];
        m_data[0] = uint8_t(m_container.size() - 1);
        for (size_t i = 1; i < m_container.size(); i++)
            m_container[i] = (uint8_t) str[i - 1];
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

    void swap(Blob& val)
    {
        std::swap(m_container, val.m_container);
        std::swap(m_data, val.m_data);
    }

private:
    std::vector<uint8_t> m_container;
};

//////////////////////////////////////////////////////////////////////////

class KeyCmp
{

public:
    KeyCmp(const uint8_t* data)
        : m_data(data)
    {}

    bool operator()(uint16_t left, uint16_t right) const
    {
        BlobRef l(m_data + left);
        BlobRef r(m_data + right);
        return l < r;
    }

    bool operator()(uint16_t left, const BlobRef& right) const
    {
        BlobRef l(m_data + left);
        return l < right;
    }

    bool operator()(const BlobRef& left, uint16_t right) const
    {
        BlobRef r(m_data + right);
        return left < r;
    }

private:
    const uint8_t* m_data;
};

}

#endif
