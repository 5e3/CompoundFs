


#pragma once
#ifndef BLOB_H
#define BLOB_H

#include <cstdint>
#include <vector>
#include <algorithm>
#include <assert.h>



namespace CompFs
{

    class Blob
    {
    public:
        Blob() : m_data(0) {}

        Blob(const char* str)
            : m_container(strlen(str) + 1)
            , m_data(&m_container[0])
        {
            assert(m_container.size() <= UINT8_MAX);
            m_container[0] = m_container.size() - 1;
            for (size_t i = 1; i < m_container.size(); i++)
                m_container[i] = (uint8_t)str[i - 1];
        }

        Blob(const uint8_t* data) : m_data((uint8_t*)data) {}

        Blob(const Blob& rhs)
            : m_container(rhs.m_container)
            , m_data(m_container.empty() ? rhs.m_data : &m_container[0])
        {
        }

        Blob& operator=(const Blob& rhs)
        {
            Blob tmp = rhs;
            swap(tmp);
            return *this;
        }

        void swap(Blob& rhs)
        {
            std::swap(m_container, rhs.m_container);
            std::swap(m_data, rhs.m_data);
        }

        void assign(const uint8_t* data)
        {
            m_container.resize(*data + 1);
            std::copy(data, data + *data + 1, m_container.begin());
            m_data = &m_container[0];
        }

        size_t size() const { return *m_data + 1; }
        const uint8_t* begin() const { return m_data; }
        const uint8_t* end() const { return begin() + size(); }
        bool operator==(const Blob& lhs) const
        {
            return std::equal(begin(), end(), lhs.begin());
        }
        bool operator!=(const Blob& lhs) const { return !(*this == lhs); }
        bool operator<(const Blob& rhs) const
        {
            return std::lexicographical_compare(begin() + 1, end(), rhs.begin() + 1, rhs.end());
        }

        uint8_t* begin() { return m_data; }
        uint8_t* end() { return begin() + size(); }


    private:
        std::vector<uint8_t> m_container;
        uint8_t* m_data;

    };


    //////////////////////////////////////////////////////////////////////////

    class KeyCmp
    {

    public:
        KeyCmp(const uint8_t* data) : m_data(data) {}

        bool operator()(uint16_t left, uint16_t right)
        {
            Blob l(m_data + left);
            Blob r(m_data + right);
            return l < r;
        }

        bool operator()(uint16_t left, const Blob& right)
        {
            Blob l(m_data + left);
            return l < right;
        }

        bool operator()(const Blob& left, uint16_t right)
        {
            Blob r(m_data + right);
            return left < r;
        }

    private:
        const uint8_t* m_data;
    };


}



#endif