
#include "RollbackHandler.h"
#include "LogPage.h"
#include "FileIo.h"
#include <assert.h>

using namespace TxFs;

RollbackHandler::RollbackHandler(Cache& cache) noexcept
    : m_cache(cache)
{}

void TxFs::RollbackHandler::revertPartialCommit()
{
    auto logs = readLogs();
    for (auto [orig, cpy]: logs)
        TxFs::copyPage(m_cache.file(), cpy, orig);
    m_cache.file()->flushFile();
}

void RollbackHandler::rollback(size_t compositeSize)
{
    m_cache.m_pageCache.clear();
    m_cache.m_newPageIds.clear();
    m_cache.m_divertedPageIds.clear();
    assert(compositeSize <= m_cache.file()->fileSizeInPages());
    auto commitLock = m_cache.commitAccess();
    m_cache.m_fileInterface->truncate(compositeSize);
    m_cache.m_lock = commitLock.release();
}

void RollbackHandler::virtualRevertPartialCommit()
{
    auto logs = readLogs();
    for (auto [orig, cpy]: logs)
        m_cache.m_divertedPageIds[orig] = cpy;
}

std::vector<std::pair<PageIndex, PageIndex>> RollbackHandler::readLogs() const
{
    std::vector<std::pair<PageIndex, PageIndex>> res;
    auto size = m_cache.m_fileInterface->fileSizeInPages();
    if (!size)
        return res;

    PageIndex idx = static_cast<PageIndex>(size);
    LogPage logPage {};

    do
    {
        bool check = TxFs::testReadSignedPage(m_cache.file(), --idx, &logPage);
        if (!check)
            return res;

        if (!logPage.checkSignature(idx))
            return res;

        res.reserve(res.size() + logPage.size());
        for (auto [orig, cpy]: logPage)
            res.emplace_back(orig, cpy);
    } while (idx != 0);

    return res;
}

