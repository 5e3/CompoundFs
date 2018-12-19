
#pragma once

#include "Node.h"

namespace CompFs
{

    struct FileDescriptor
    {
        Node::Id m_first;
        Node::Id m_last;
        uint64_t m_fileSize;

        FileDescriptor()
            : m_first(Node::INVALID_NODE)
            , m_last(Node::INVALID_NODE)
            , m_fileSize(0)
        {
        }

        explicit FileDescriptor(Node::Id singlePage)
            : m_first(singlePage)
            , m_last(singlePage)
            , m_fileSize(0)
        {
        }

        bool operator==(const FileDescriptor& rhs) const
        {
            return m_first == rhs.m_first
                && m_last == rhs.m_last
                && m_fileSize == rhs.m_fileSize;
        }

        bool operator!=(const FileDescriptor& rhs) const { return !(*this == rhs); }
    };

}