

#pragma once

#include "Node.h"
#include "PageAllocator.h"
#include <utility>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace TxFs
{
class RawFileInterface
{
public:
    virtual ~RawFileInterface() {}

    virtual Node::Id newPage() = 0;
    virtual void writePage(Node::Id id, std::shared_ptr<uint8_t> page) = 0;
    virtual void readPage(Node::Id id, std::shared_ptr<uint8_t> page) const = 0;
};

///////////////////////////////////////////////////////////////////////////

class CacheManager
{
    typedef std::pair<std::shared_ptr<uint8_t>, Node::Id> Page;

    struct PageMetaData
    {
        enum { Undefined, Read, New, DirtyRead };

        unsigned int m_type : 2;
        unsigned int m_usageCount : 24;
        unsigned int m_priority : 6;

        PageMetaData(int type, int priority);
    };

    struct CachedPage : PageMetaData
    {
        std::shared_ptr<uint8_t> m_page;

        CachedPage(const std::shared_ptr<uint8_t>& page, int type, int priority = 0);
    };

public:
    struct PageSortItem : PageMetaData
    {
        Node::Id m_id;

        PageSortItem(const PageMetaData& pmd, Node::Id id);
        PageSortItem(int type, int usageCount, int priority, Node::Id id);

        bool operator<(PageSortItem rhs) const;
        bool operator==(PageSortItem rhs) const;
    };

public:
    CacheManager(RawFileInterface* rfi, uint32_t maxPages = 256);

    Page newPage();
    std::shared_ptr<uint8_t> getPage(Node::Id id);
    void pageDirty(Node::Id id);
    size_t trim(uint32_t maxPages);

private:
    Node::Id redirectPage(Node::Id id) const;
    std::vector<PageSortItem> getUnpinnedPages() const;

    void trimCheck();
    void evictDirtyPages(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end);
    void evictNewPages(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end);
    void removeFromCache(std::vector<PageSortItem>::iterator begin, std::vector<PageSortItem>::iterator end);

private:
    RawFileInterface* m_rawFileInterface;
    std::unordered_map<Node::Id, CachedPage> m_cache;
    std::unordered_map<Node::Id, Node::Id> m_redirectedPagesMap;
    std::unordered_set<Node::Id> m_newPageSet;
    PageAllocator m_pageAllocator;
    uint32_t m_maxPages;
};

}
