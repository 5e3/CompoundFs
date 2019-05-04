
#pragma once

#include "Node.h"

namespace TxFs
{

struct FileDescriptor
{
    PageIndex m_first;
    PageIndex m_last;
    uint64_t m_fileSize;

    constexpr FileDescriptor() noexcept
        : m_first(PageIdx::INVALID)
        , m_last(PageIdx::INVALID)
        , m_fileSize(0)
    {}

    constexpr FileDescriptor(PageIndex first, PageIndex last, uint64_t fileSize) noexcept
        : m_first(first)
        , m_last(last)
        , m_fileSize(fileSize)
    {}

    constexpr explicit FileDescriptor(PageIndex singlePage) noexcept
        : m_first(singlePage)
        , m_last(singlePage)
        , m_fileSize(0)
    {}

    constexpr bool operator==(const FileDescriptor& rhs) const noexcept
    {
        return m_first == rhs.m_first && m_last == rhs.m_last && m_fileSize == rhs.m_fileSize;
    }

    constexpr bool operator!=(const FileDescriptor& rhs) const noexcept { return !(*this == rhs); }
};

}
