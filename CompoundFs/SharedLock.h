
#pragma once

#include <mutex>
#include <condition_variable>
#include <cassert>

namespace TxFs
{

///////////////////////////////////////////////////////////////////////////////

    class SharedLock
{
    std::mutex m_mutex;
    std::condition_variable m_cv;

    bool m_exclusive;
    int m_shared;

public:
    void lock();
    bool try_lock();
    void unlock();

    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();
};

///////////////////////////////////////////////////////////////////////////////

class DebugSharedLock
{
    struct Copyable
    {
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;

        bool m_exclusive = false;
        int m_shared = 0;
        int m_waiting = 0;
        bool m_unlockRelease = true;
    };

    std::shared_ptr<Copyable> m_state = std::make_shared<Copyable>();


public:
    void lock();
    bool try_lock();
    void unlock();

    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();

    int getWaiting() const;
    void waitForWaiting(int waiting);
    void unlockRelease();
    void makeUnlockBlock();
};


///////////////////////////////////////////////////////////////////////////////

inline void SharedLock::lock()
{
    {
        std::unique_lock ul(m_mutex);
        while (m_exclusive || m_shared > 0)
            m_cv.wait(ul);
        m_exclusive = true;
    }
}

inline bool SharedLock::try_lock()
{
    {
        std::unique_lock ul(m_mutex);
        if (m_exclusive || m_shared > 0)
            return false;
        m_exclusive = true;
    }
    return true;
}

inline void SharedLock::unlock()
{
    {
        std::unique_lock ul(m_mutex);
        assert(m_exclusive && m_shared == 0);
        m_exclusive = false;
    }
    m_cv.notify_all();
}

inline void SharedLock::lock_shared()
{
    {
        std::unique_lock ul(m_mutex);
        while (m_exclusive)
            m_cv.wait(ul);
        assert(!m_exclusive && m_shared >= 0);
        m_shared++;
    }
}

inline bool SharedLock::try_lock_shared()
{
    {
        std::unique_lock ul(m_mutex);
        if (m_exclusive)
            return false;
        assert(!m_exclusive && m_shared >= 0);
        m_shared++;
    }
    return true;
}

inline void SharedLock::unlock_shared()
{
    {
        std::unique_lock ul(m_mutex);
        m_shared--;
        assert(!m_exclusive && m_shared >= 0);
    }
    if (m_shared == 0)
        m_cv.notify_all();
}

///////////////////////////////////////////////////////////////////////////////


}


