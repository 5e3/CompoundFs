
#pragma once

#include <vector>
#include "Blob.h"
#include "FixupTable.h"
#include "PushBits.h"
#include <type_traits>
#include <ranges>
#include <stdexcept>

namespace Rfx
{
template <typename T>
struct ForEachMember
{
    using Versioned = bool;
};


template<typename T>
concept VersionedStructure = requires 
{ 
    typename ForEachMember<T>::Versioned;
    requires std::is_class_v<T>;
};

template<typename T>
concept FixedStructure = std::is_class_v<T> && !VersionedStructure<T>;

class StreamOut
{
    Blob m_blob;
    FixupTable m_fixups;

public:
    template<typename T>
    void write(const T& value)
    {
        if constexpr (FixedStructure<T>)
        {
            ForEachMember<T>::write(value, *this);
        }
        else if constexpr (VersionedStructure<T>)
        {
            size_t topIndex = m_fixups.size();
            auto fixup = m_fixups.nextFixup(m_blob.size());
            forEachMember((T&) value, [this](const auto& val) { this->write(val); });
            fixup(m_blob.size());
            write(CompressedInteger<size_t> { m_fixups.size() - topIndex });
        }
        else
        {
            static_assert(BitStreamable<T>);
            pushBits(value, m_blob);
        }
    }

    template<typename TRange>
    void writeRange(const TRange& range)
    {
        if constexpr (BitStreamable<std::ranges::range_value_t<TRange>>)
            pushBits(range, m_blob);
        else
        {
            for (auto it = range.begin(); it != range.end(); ++it)
                write(*it);
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
        if constexpr (FixedStructure<T>)
        {
            ForEachMember<T>::read(value, *this);
        }
        else if constexpr (VersionedStructure<T>)
        {
            auto last = m_last;
            auto currentFixup = m_currentFixup;
            m_currentFixup = advanceBy(m_currentFixup, 1);
            m_last = m_first + *currentFixup;
            assert(m_last <= last);
            forEachMember(value, [this](auto& val) 
            {
                if (this->m_first < this->m_last)
                    this->read(val); 
                });
            assert(m_first <= m_last);
            // m_first = m_last;
            m_last = last;
            CompressedInteger<size_t> advance {};
            read(advance);
            m_currentFixup = advanceBy(currentFixup, advance.m_value);
        }
        else
        {
            static_assert(BitStreamable<T>);
            m_first = popBits(value, asRange());
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
};

}