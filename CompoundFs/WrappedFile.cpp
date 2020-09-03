
#include "WrappedFile.h"
#include "Lock.h"

using namespace TxFs;


Interval WrappedFile::newInterval(size_t maxPages)
{
    return m_wrappedFile->newInterval(maxPages);
}

const uint8_t* WrappedFile::writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    return m_wrappedFile->writePage(id, pageOffset, begin, end);
}

const uint8_t* WrappedFile::writePages(Interval iv, const uint8_t* page)
{
    return m_wrappedFile->writePages(iv, page);
}

uint8_t* WrappedFile::readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    return m_wrappedFile->readPage(id, pageOffset, begin, end);
}

uint8_t* WrappedFile::readPages(Interval iv, uint8_t* page) const
{
    return m_wrappedFile->readPages(iv, page);
}

size_t WrappedFile::currentSize() const
{
    return m_wrappedFile->currentSize();
}

void WrappedFile::flushFile()
{
    m_wrappedFile->flushFile();
}

void WrappedFile::truncate(size_t numberOfPages)
{
    m_wrappedFile->truncate(numberOfPages);
}

Lock WrappedFile::defaultAccess()
{
    return m_wrappedFile->defaultAccess();
}

Lock WrappedFile::readAccess()
{
    return m_wrappedFile->readAccess();
}

Lock WrappedFile::writeAccess()
{
    return m_wrappedFile->writeAccess();
}

CommitLock WrappedFile::commitAccess(Lock&& writeLock)
{
    return m_wrappedFile->commitAccess(std::move(writeLock));
}
