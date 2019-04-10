
#pragma once

#include "../CompoundFs/RawFileInterface.h"
#include "../CompoundFs/CacheManager.h"
#include "../CompoundFs/TypedCacheManager.h"
#include "../CompoundFs/InnerNode.h"
#include "../CompoundFs/Leaf.h"

namespace TxFs
{
struct MinimalTreeBuilder
{
    RawFileInterface* m_file;
    std::shared_ptr<CacheManager> m_cacheManager;
    TypedCacheManager m_typedCacheManager;
    int m_leafKey;
    struct NodeDesc
    {
        int m_min;
        int m_max;
        PageIndex m_index;
    };

    MinimalTreeBuilder(RawFileInterface* rfi)
        : m_file(rfi)
        , m_cacheManager(std::make_shared<CacheManager>(rfi))
        , m_typedCacheManager(m_cacheManager)
        , m_leafKey(1)
    {}

    PageIndex buildTree(int depth)
    {
        auto left = makeNode(depth - 1);
        auto right = makeNode(depth - 1);
        auto root = m_typedCacheManager.newPage<InnerNode>(makeKey((left.m_max + right.m_min) / 2), left.m_index,
                                                           right.m_index);
        root.m_page.reset();     // release root page
        m_cacheManager->trim(0); // force writing to file
        return root.m_index;
    }

    NodeDesc makeNode(int depth)
    {
        if (depth == 1)
        {
            auto leaf = m_typedCacheManager.newPage<Leaf>();
            auto key = makeKey(m_leafKey);
            m_leafKey += 2;
            leaf.m_page->insert(key, key);
            return { m_leafKey - 2, m_leafKey - 2, leaf.m_index };
        }

        auto left = makeNode(depth - 1);
        auto middle = makeNode(depth - 1);
        int leftKey = (left.m_max + middle.m_min) / 2;
        auto node = m_typedCacheManager.newPage<InnerNode>(makeKey(leftKey), left.m_index, middle.m_index);

        auto right = makeNode(depth - 1);
        int rightKey = (middle.m_max + right.m_min) / 2;
        node.m_page->insert(makeKey(rightKey), right.m_index);
        return { leftKey, rightKey, node.m_index };
    }

    ByteString makeKey(int i)
    {
        char buf[10];
        sprintf_s(buf, "%04d", i);
        return ByteString(buf);
    }
};
}
