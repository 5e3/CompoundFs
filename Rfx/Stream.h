
#pragma once

#include <vector>
#include "Blob.h"
#include "FixupTable.h"
#include "PushBits.h"
#include "StreamRule.h"
#include <type_traits>
#include <ranges>
#include <stdexcept>

namespace Rfx
{

class StreamOut
{
    Blob m_blob;
    FixupTable m_fixups;

public:
    using SizeType = CompressedInteger<size_t>;

public:
    template<typename T>
    void write(const T& value)
    {
        if constexpr (BitStreamable<T>)
            pushBits(value, m_blob);
        else if constexpr (isVersioned<T>)
        {
            size_t topIndex = m_fixups.size();
            auto fixup = m_fixups.nextFixup(m_blob.size());
            StreamRule<T>::write(value, *this);
            fixup(m_blob.size());
            write(SizeType { m_fixups.size() - topIndex });
        }
        else
        {
            StreamRule<T>::write(value, *this);
        }
    }

    template <typename T>
    void operator()(const T& value)
    {
        write(value);
    }

    template<typename TRange>
    void writeRange(const TRange& range)
    {
        if constexpr (BitStreamable<std::ranges::range_value_t<TRange>>)
            pushBits(range, m_blob);
        else
        {
            for (const auto& val: range)
                write(val);
        }
    }

    Blob swapBlob(Blob&& blob = Blob())
    {
        storeFixups();
        auto ret = std::move(m_blob);
        m_blob = std::move(blob);
        return ret;
    }

private:
    void storeFixups() 
    { 
        auto size =  m_fixups.sizeInBytes();
        [[maybe_unused]] auto last = std::make_reverse_iterator(m_blob.grow(size));
        auto first = m_blob.rbegin();
        [[maybe_unused]] auto last2 = m_fixups.write(first);
        assert(last == last2);
    }
};


class StreamIn 
{
    Blob::const_iterator m_first;
    Blob::const_iterator m_last;
    FixupTable m_fixups;
    FixupTable::iterator m_currentFixup;

    std::ranges::subrange<Blob::const_iterator> asRange() const { return std::ranges::subrange(m_first, m_last); }
    FixupTable::iterator advanceBy(FixupTable::iterator it, size_t steps) const
    {
        if (m_fixups.end() < (it + steps))
            throw std::out_of_range("advanceBy()");
        return it + steps;
    }

public:
    using SizeType = CompressedInteger<size_t>;

public:
    StreamIn(const Blob& blob)
        : m_first(blob.begin())
        , m_last(blob.end())
    {
        auto it = m_fixups.read(blob.rbegin(), blob.rend());
        m_last = it.base();
        m_currentFixup = m_fixups.begin();
    }

    template<typename T>
    void read(T& value)
    {
        if constexpr (BitStreamable<T>)
        {
            m_first = popBits(value, asRange());
        }
        else if constexpr (isVersioned<T>)
        {
            auto last = m_last;
            auto currentFixup = m_currentFixup;
            m_currentFixup = advanceBy(m_currentFixup, 1);
            m_last = m_first + *currentFixup;
            assert(m_last <= last);
            StreamRule<T>::read(value, *this);
            assert(m_first <= m_last);
            m_first = m_last;
            m_last = last;
            SizeType advance {};
            read(advance);
            m_currentFixup = advanceBy(currentFixup, advance);
        }
        else
        {
            StreamRule<T>::read(value, *this);
        }
    }

    template <std::ranges::range TRange>
    void readRange(TRange& range)
    {
        if constexpr (BitStreamable<std::ranges::range_value_t<TRange>>)
            m_first = popBits(range, asRange());
        else
        {
            for (auto& value: range)
                read(value);
        }
    }

    template<typename T>
    void operator()(T& value)
    {
        if (m_first < m_last)
            read(value);
    }
};

}