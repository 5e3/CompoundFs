

#pragma once

#include "Node.h"
#include <memory>
#include <tuple>
#include <stdint.h>

namespace TxFs
{
///////////////////////////////////////////////////////////////////////////
/// The PageClass controls the cache-eviction protocol of the CacheManager
/// and defines how pages are persisted during the commit-phase.
enum class PageClass : uint8_t {
    Undefined,
    Read, // page was read
    New,  // previously unused page, always modified
    Dirty // modified, previously used page
};

///////////////////////////////////////////////////////////////////////////

struct PageMetaData
{
    unsigned int m_pageClass : 2;
    unsigned int m_usageCount : 24;
    unsigned int m_priority : 6;

    constexpr PageMetaData(PageClass pageClass, int priority) noexcept;
    constexpr void setPageClass(PageClass pageClass) noexcept;
};

///////////////////////////////////////////////////////////////////////////

struct CachedPage final : PageMetaData
{
    std::shared_ptr<uint8_t> m_page;

    CachedPage(const std::shared_ptr<uint8_t>& page, PageClass pageClass, int priority = 0);
};

///////////////////////////////////////////////////////////////////////////

struct PrioritizedPage final : PageMetaData
{
    PageIndex m_id;

    constexpr PrioritizedPage(const PageMetaData& pmd, PageIndex id) noexcept;
    constexpr PrioritizedPage(PageClass pageClass, int usageCount, int priority, PageIndex id) noexcept;

    constexpr bool operator<(PrioritizedPage rhs) const noexcept;
    constexpr bool operator==(PrioritizedPage rhs) const noexcept;
};

///////////////////////////////////////////////////////////////////////////////

inline constexpr PageMetaData::PageMetaData(PageClass pageClass, int priority) noexcept
    : m_pageClass(static_cast<unsigned>(pageClass))
    , m_usageCount(0)
    , m_priority(priority)
{}

constexpr void PageMetaData::setPageClass(PageClass pageClass) noexcept
{
    m_pageClass = static_cast<unsigned>(pageClass);
}

///////////////////////////////////////////////////////////////////////////////

inline CachedPage::CachedPage(const std::shared_ptr<uint8_t>& page, PageClass pageClass, int priority)
    : PageMetaData(pageClass, priority)
    , m_page(page)
{}

///////////////////////////////////////////////////////////////////////////////

inline constexpr bool operator==(unsigned lhs, PageClass rhs)
{
    return lhs == static_cast<unsigned>(rhs);
}

inline constexpr bool operator==(PageClass lhs, unsigned rhs)
{
    return rhs == lhs;
}

inline constexpr bool operator!=(unsigned lhs, PageClass rhs)
{
    return !(lhs == rhs);
}

inline constexpr bool operator!=(PageClass lhs, unsigned rhs)
{
    return !(rhs == lhs);
}

///////////////////////////////////////////////////////////////////////////////

inline constexpr PrioritizedPage::PrioritizedPage(const PageMetaData& pmd, PageIndex id) noexcept
    : PageMetaData(pmd)
    , m_id(id)
{}

inline constexpr PrioritizedPage::PrioritizedPage(PageClass pageClass, int usageCount, int priority,
                                                  PageIndex id) noexcept
    : PageMetaData(pageClass, priority)
    , m_id(id)
{
    m_usageCount = usageCount;
}

inline constexpr bool PrioritizedPage::operator<(const PrioritizedPage rhs) const noexcept
{
    return std::tie(m_pageClass, m_usageCount, m_priority)
           > std::tie(rhs.m_pageClass, rhs.m_usageCount, rhs.m_priority);
}

inline constexpr bool PrioritizedPage::operator==(const PrioritizedPage rhs) const noexcept
{
    return std::tie(m_pageClass, m_usageCount, m_priority)
           == std::tie(rhs.m_pageClass, rhs.m_usageCount, rhs.m_priority);
}

}
