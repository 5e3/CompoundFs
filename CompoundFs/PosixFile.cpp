
#include <sys/types.h>
#include "PosixFile.h"
#include "Lock.h"

#ifndef _WINDOWS
#include <unistd.h>
#else
#include <io.h>
#endif


using namespace TxFs;

PosixFile::PosixFile()
{}

PosixFile::~PosixFile()
{
    close();
}

void PosixFile::close()
{
    ::close(m_file);
}

PosixFile& PosixFile::operator=(PosixFile&&)
{
    throw std::logic_error("The method or operation is not implemented.");
}

PosixFile::PosixFile(PosixFile&&)
{}

PosixFile::PosixFile(std::filesystem::path path, OpenMode mode)
{ 
    //m_file = ::open(path.c_str(),  
}


Interval PosixFile::newInterval(size_t maxPages)
{
    throw std::logic_error("The method or operation is not implemented.");
}

const uint8_t* PosixFile::writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end)
{
    throw std::logic_error("The method or operation is not implemented.");
}

const uint8_t* PosixFile::writePages(Interval iv, const uint8_t* page)
{
    throw std::logic_error("The method or operation is not implemented.");
}

uint8_t* PosixFile::readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const
{
    throw std::logic_error("The method or operation is not implemented.");
}

uint8_t* PosixFile::readPages(Interval iv, uint8_t* page) const
{
    throw std::logic_error("The method or operation is not implemented.");
}

size_t PosixFile::currentSize() const
{
    throw std::logic_error("The method or operation is not implemented.");
}

void PosixFile::flushFile()
{
    throw std::logic_error("The method or operation is not implemented.");
}

void PosixFile::truncate(size_t numberOfPages)
{
    throw std::logic_error("The method or operation is not implemented.");
}

Lock PosixFile::defaultAccess()
{
    throw std::logic_error("The method or operation is not implemented.");
}

Lock PosixFile::readAccess()
{
    throw std::logic_error("The method or operation is not implemented.");
}

Lock PosixFile::writeAccess()
{
    throw std::logic_error("The method or operation is not implemented.");
}

CommitLock PosixFile::commitAccess(Lock&& writeLock)
{
    throw std::logic_error("The method or operation is not implemented.");
}

