

#pragma once

#include <vector>
#include <ranges>
#include "CompressedInteger.h"

namespace Rfx
{
class FixupTable
{
    std::vector<size_t> m_fixups;
    size_t m_sizeInBytes = 0;
    template <typename TIter> struct CompressedWriter;
    template <typename TIter> struct CompressedReader;

public:
    using iterator = std::vector<size_t>::const_iterator;

    iterator begin() const { return m_fixups.begin(); }
    iterator end() const { return m_fixups.end(); }
    std::strong_ordering operator<=>(const FixupTable&) const noexcept = default;

    auto nextFixup(size_t currentPos)
    {
        m_fixups.push_back(std::numeric_limits<size_t>::max());
        return [start = currentPos, idx = m_fixups.size() - 1, this](size_t last) {
            if (this->m_fixups[idx] != last - start)
            {
                this->m_fixups[idx] = last - start;
                this->m_sizeInBytes += compressedSize(last - start);
            }
        };
   }

    size_t sizeInBytes() const noexcept { return m_sizeInBytes + compressedSize(m_fixups.size()); }
    size_t size() const noexcept { return m_fixups.size(); }

    template<typename TIterator>
    TIterator write(TIterator it)
    {
        CompressedWriter<TIterator> out { it };
        out.write(m_fixups.size());
        for (auto f: m_fixups)
            out.write(f);
        return out.m_iter;
    }

    template<typename TIterator> 
    TIterator read(TIterator first, TIterator last)
    {
        CompressedReader<TIterator> in { first, last };
        size_t size {};
        in.read(size);
        m_fixups.resize(size);
        for (auto& f: m_fixups)
            in.read(f);
        m_sizeInBytes = (in.m_first-first) - compressedSize(size);
        return in.m_first;
    }
};

template <typename TIter>
struct FixupTable::CompressedWriter
{
    TIter m_iter;
    void write(size_t value) { ForEachMember<CompressedInteger<size_t>>::write({ value }, *this); }
    void write(std::byte value)
    {
        *m_iter = value;
        ++m_iter;
    }
};

template <typename TIterator>
struct FixupTable::CompressedReader
{
    TIterator m_first;
    TIterator m_last;

    void read(size_t& value)
    {
        CompressedInteger<size_t> ci {};
        ForEachMember<CompressedInteger<size_t>>::read(ci, *this);
        value = ci.m_value;
    }
    void read(std::byte& value)
    {
        if (m_first == m_last)
            throw std::out_of_range("CompressedReader");
        value = *m_first++;
    }
};
}