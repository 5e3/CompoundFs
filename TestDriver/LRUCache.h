

#pragma once
#ifndef LRUCACHE_H
#define LRUCACHE_H

namespace CompFs
{
    class LRUCache
    {
    public:


    private:
        struct Priority { enum Value {Read0, Read1, Dirty, Last}; };
        struct CachedPage
        {
            Node* m_node;
            std::list<Node::Id>::iterator m_pos;
            Priority::Value m_priority;
        };
        std::list<Node::Id> m_lru[Priority::Last];
        std::unordered_map<Node::Id, CachedPage> m_pages;
    };

}



#endif