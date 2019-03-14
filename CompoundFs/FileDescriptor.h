
#pragma once

#include "Node.h"

namespace TxFs
{

struct FileDescriptor
{
    PageIndex m_first;
    PageIndex m_last;
    uint64_t m_fileSize;

    FileDescriptor()
        : m_first(PageIdx::INVALID)
        , m_last(PageIdx::INVALID)
        , m_fileSize(0)
    {}

    FileDescriptor(PageIndex first, PageIndex last, uint64_t fileSize)
        : m_first(first)
        , m_last(last)
        , m_fileSize(fileSize)
    {}

    explicit FileDescriptor(PageIndex singlePage)
        : m_first(singlePage)
        , m_last(singlePage)
        , m_fileSize(0)
    {}

    bool operator==(const FileDescriptor& rhs) const
    {
        return m_first == rhs.m_first && m_last == rhs.m_last && m_fileSize == rhs.m_fileSize;
    }

    bool operator!=(const FileDescriptor& rhs) const { return !(*this == rhs); }
};

}
