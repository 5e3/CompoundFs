

#include  "SharedLock.h"

using namespace TxFs;


void DebugSharedLock::lock()
{
    std::unique_lock ul(m_state->m_mutex);
    m_state->m_waiting++;
    m_state->m_cv.notify_all();

    while (m_state->m_exclusive || m_state->m_shared > 0)
        m_state->m_cv.wait(ul);
    m_state->m_exclusive = true;
    m_state->m_waiting--;
    m_state->m_cv.notify_all();
}

bool DebugSharedLock::try_lock()
{
    std::unique_lock ul(m_state->m_mutex);
    if (m_state->m_exclusive || m_state->m_shared > 0)
        return false;
    m_state->m_exclusive = true;

    return true;
}

void DebugSharedLock::unlock()
{
    std::unique_lock ul(m_state->m_mutex);
    assert(m_state->m_exclusive && m_state->m_shared == 0);
    m_state->m_exclusive = false;
    m_state->m_cv.notify_all();
    while (m_state->m_unlockRelease == false)
        m_state->m_cv.wait(ul);
}

void DebugSharedLock::lock_shared()
{
    std::unique_lock ul(m_state->m_mutex);
    m_state->m_waiting++;
    m_state->m_cv.notify_all();

    while (m_state->m_exclusive)
        m_state->m_cv.wait(ul);
    assert(!m_state->m_exclusive && m_state->m_shared >= 0);
    m_state->m_shared++;
    m_state->m_waiting--;
    m_state->m_cv.notify_all();
}

bool DebugSharedLock::try_lock_shared()
{
    std::unique_lock ul(m_state->m_mutex);
    if (m_state->m_exclusive)
        return false;
    assert(!m_state->m_exclusive && m_state->m_shared >= 0);
    m_state->m_shared++;

    return true;
}

void DebugSharedLock::unlock_shared()
{
    {
        std::unique_lock ul(m_state->m_mutex);
        m_state->m_shared--;
        assert(!m_state->m_exclusive && m_state->m_shared >= 0);
    }
    if (m_state->m_shared == 0)
        m_state->m_cv.notify_all();
}

int DebugSharedLock::getWaiting() const
{
    std::unique_lock ul(m_state->m_mutex);
    return m_state->m_waiting;
}

void DebugSharedLock::waitForWaiting(int waiting)
{
    std::unique_lock ul(m_state->m_mutex);
    while (m_state->m_waiting != waiting)
        m_state->m_cv.wait(ul);
}

void DebugSharedLock::unlockRelease()
{
    {
        std::unique_lock ul(m_state->m_mutex);
        m_state->m_unlockRelease = true;
    }
    m_state->m_cv.notify_all();
}

void DebugSharedLock::makeUnlockBlock()
{
    std::unique_lock ul(m_state->m_mutex);
    m_state->m_unlockRelease = false;
}


