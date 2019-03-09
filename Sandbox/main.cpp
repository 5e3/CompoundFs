
#include "../CompoundFs/InnerNode.h"
#include "../CompoundFs/PageAllocator.h"
#include <memory>
#include <utility>
#include <iostream>

using namespace TxFs;

struct Reporter
{
    Reporter()
    {
        std::cout << "ctor\n";
    }

    Reporter(const Reporter&)
    {
        std::cout << "copy ctor\n";
    }

    Reporter(Reporter&&)
    {
        std::cout << "move ctor\n";
    }

    ~Reporter()
    {
        std::cout << "dtor\n";
    }

    Reporter& operator=(const Reporter&)
    {
        std::cout << "copy assign\n";
        return *this;
    }

    Reporter& operator=(const Reporter&&)
    {
        std::cout << "move assign\n";
        return *this;
    }


};


class BlobRef : private Reporter
{
public:
    BlobRef() : m_data(0)
    {
    }

    BlobRef(const uint8_t* val)
        : m_data((uint8_t*) val)
    {}

    uint16_t size() const { return *m_data + 1; }
    const uint8_t* begin() const { return m_data; }
    const uint8_t* end() const { return begin() + size(); }
    bool operator==(const Blob& lhs) const { return std::equal(begin(), end(), lhs.begin()); }
    bool operator!=(const Blob& lhs) const { return !(*this == lhs); }
    bool operator<(const Blob& rhs) const
    {
        return std::lexicographical_compare(begin() + 1, end(), rhs.begin() + 1, rhs.end());
    }

    uint8_t* begin() { return m_data; }
    uint8_t* end() { return begin() + size(); }


protected:
    uint8_t* m_data;
};

class BlobVal : public BlobRef
{
public:
    BlobVal() = default;

    BlobVal(const char* str)
        : m_container(strlen(str) + 1)
    {
        m_data = &m_container[0];
        m_data[0] = uint8_t(m_container.size()-1);
        for (size_t i = 1; i < m_container.size(); i++)
            m_container[i] = (uint8_t)str[i - 1];
    }

    BlobVal(const uint8_t* val)
        : m_container(*val + 1)
    {
        m_data = &m_container[0];
        std::copy(val, val + m_container.size(), m_data);
    }

    BlobVal(const BlobRef& br)
        : BlobVal(br.begin())
    {
    }

    BlobVal(const BlobVal& val)
        : m_container(val.m_container)
    {
        m_data = &m_container[0];
    }

    BlobVal(BlobVal&& val)
        : m_container(std::move(val.m_container))
    {
        m_data = &m_container[0];
    }

    BlobVal& operator=(const BlobVal& val)
    {
        auto tmp = val;
        swap(tmp);
        return *this;
    }

    BlobVal& operator=(const BlobRef& ref)
    {
        BlobVal val(ref);
        swap(val);
        return *this;
    }

    BlobVal& operator=(BlobVal&& val)
    {
        m_container = std::move(val.m_container);
        m_data = &m_container[0];
        return *this;
    }

    void swap(BlobVal& val)
    {
        std::swap(m_container, val.m_container);
        std::swap(m_data, val.m_data);
    }

private:
    std::vector<uint8_t> m_container;
};

BlobVal creatBlobVal(int i=0)
{
    if (i)
        return "Hello";
    else
        return BlobVal("World");
}

BlobVal doSome(BlobVal val)
{
    return val;
}




int main()
{


    auto a = doSome(creatBlobVal());
    a = doSome(creatBlobVal());

}
