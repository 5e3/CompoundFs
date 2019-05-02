

#pragma once
#ifndef NODE_H
#define NODE_H

#include <cstdint>

namespace TxFs
{

using PageIndex = uint32_t;
enum PageIdx : PageIndex { INVALID = UINT32_MAX };

enum class NodeType : uint8_t { Undefined, Leaf, Inner };

//////////////////////////////////////////////////////////////////////////

#pragma pack(push)
#pragma pack(1)

struct Node
{
    uint16_t m_begin;
    uint16_t m_end;
    NodeType m_type;

    constexpr Node(uint16_t begin = 0, uint16_t end = 0, NodeType type = NodeType::Undefined) noexcept
        : m_begin(begin)
        , m_end(end)
        , m_type(type)
    {}
};

//////////////////////////////////////////////////////////////////////////

#pragma pack(pop)
}

#endif
