

#pragma once

#include <algorithm>

namespace TxFs
{

class Lock final
{
private:
    using ReleaseFunc = void (*)(void*);

public:
    constexpr Lock() noexcept = default;
    template <typename TMutex>
    constexpr Lock(TMutex* mutex, ReleaseFunc releaseFunc) noexcept;
    constexpr Lock(Lock&&) noexcept;
    ~Lock();
    constexpr Lock& operator=(Lock&&) noexcept;
    constexpr Lock& operator=(nullptr_t) noexcept;

    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;

    void swap(Lock& other) noexcept;
    constexpr void release() noexcept;
    template <typename TMutex>
    constexpr bool isSameMutex(const TMutex* other) const noexcept;

private:
    void* m_mutex = nullptr;
    ReleaseFunc m_releaseFunc = nullptr;
};

///////////////////////////////////////////////////////////////////////////

class CommitLock final
{
public:
    constexpr CommitLock() noexcept = default;
    constexpr CommitLock(Lock&& exclusive, Lock&& shared) noexcept;
    constexpr CommitLock(CommitLock&&) noexcept = default;
    ~CommitLock() = default;

    Lock release() noexcept; // returns m_exclusiveLock

private:
    Lock m_exclusiveLock;
    Lock m_sharedLock;
};

///////////////////////////////////////////////////////////////////////////

constexpr CommitLock::CommitLock(Lock&& exclusive, Lock&& shared) noexcept
    : m_exclusiveLock(std::move(exclusive))
    , m_sharedLock(std::move(shared))
{}

inline Lock CommitLock::release() noexcept
{
    m_sharedLock.release();
    return std::move(m_exclusiveLock);
}

///////////////////////////////////////////////////////////////////////////

template <typename TMutex>
constexpr Lock::Lock(TMutex* mutex, ReleaseFunc releaseFunc) noexcept
    : m_mutex(mutex)
    , m_releaseFunc(releaseFunc)
{}

constexpr Lock::Lock(Lock&& rhs) noexcept
{
    m_mutex = rhs.m_mutex;
    m_releaseFunc = rhs.m_releaseFunc;
    rhs.m_mutex = nullptr;
    rhs.m_releaseFunc = nullptr;
}

inline Lock::~Lock()
{
    release();
}

constexpr Lock& Lock::operator=(Lock&& rhs) noexcept
{
    m_mutex = rhs.m_mutex;
    m_releaseFunc = rhs.m_releaseFunc;
    rhs.m_mutex = nullptr;
    rhs.m_releaseFunc = nullptr;
    return *this;
}

constexpr Lock& Lock::operator=(nullptr_t) noexcept
{
    release();
    return *this;
}

inline void Lock::swap(Lock& other) noexcept
{
    Lock tmp = std::move(*this);
    *this = std::move(other);
    other = std::move(tmp);
}

constexpr void Lock::release() noexcept
{
    if (m_releaseFunc)
        m_releaseFunc(m_mutex);
    m_mutex = nullptr;
    m_releaseFunc = nullptr;
}

template <typename TMutex>
constexpr bool Lock::isSameMutex(const TMutex* other) const noexcept
{
    const void* m = other;
    return m == m_mutex;
}

}
