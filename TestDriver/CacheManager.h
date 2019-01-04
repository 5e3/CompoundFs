

#pragma once

#include "Node.h"
#include <utility>
#include <memory>

namespace CompFs
{
    class RawFileInterface
    {
    public:
        virtual ~RawFileInterface() {}

        virtual Node::Id newNode() = 0;
        virtual void writeNode(Node*) = 0;
        virtual void readNode(Node::Id id, Node*) const = 0;
    };

    ///////////////////////////////////////////////////////////////////////////

    class CacheManager
    {
        typedef std::pair<std::shared_ptr<Node>, Node::Id> NodePage;

    public:
        CacheManager(RawFileInterface* rfi, uint32_t highWaterMark);

        NodePage newNode();
        void pageDirty(Node::Id id);
        std::shared_ptr<Node> getPage(Node::Id id);



    };

}
