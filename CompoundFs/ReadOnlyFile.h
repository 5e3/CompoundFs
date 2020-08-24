
#pragma once

#include "FileInterface.h"
#include <stdexcept>
#include <utility>

namespace TxFs
{

///////////////////////////////////////////////////////////////////////////////
/// Decorator for FileInterface derivatives. Throws an exception on every write operation.
template <typename TFile>
class ReadOnlyFile : public TFile
{
public:
    ReadOnlyFile() = default;
    ReadOnlyFile(TFile&& file);
    template<typename... Ts>
    ReadOnlyFile(Ts&&...);

    Interval newInterval(size_t maxPages) override;
    const uint8_t* writePage(PageIndex idx, size_t pageOffset, const uint8_t* begin, const uint8_t* end) override;
    const uint8_t* writePages(Interval iv, const uint8_t* page) override;
    void flushFile() override;
    void truncate(size_t numberOfPages) override;

    Lock defaultAccess() override;
    Lock writeAccess() override;
    CommitLock commitAccess(Lock&& writeLock) override;
};

///////////////////////////////////////////////////////////////////////////////

struct IllegalWriteOperation : public std::runtime_error
{
    IllegalWriteOperation() 
        : std::runtime_error("Illegal write operation") 
    {}
};

///////////////////////////////////////////////////////////////////////////////
template <typename TFile>
ReadOnlyFile<TFile>::ReadOnlyFile(TFile&& file)
    : TFile(std::move(file))
{}

template <typename TFile>
template <typename... Ts>
ReadOnlyFile<TFile>::ReadOnlyFile(Ts&&... args)
    : TFile(std::forward<Ts>(args...))
{}

template <typename TFile>
Interval ReadOnlyFile<TFile>::newInterval(size_t)
{
    throw IllegalWriteOperation();
}

template <typename TFile>
const uint8_t* ReadOnlyFile<TFile>::writePage(PageIndex, size_t, const uint8_t*,
                                              const uint8_t*)
{
    throw IllegalWriteOperation();
}

template <typename TFile>
const uint8_t* ReadOnlyFile<TFile>::writePages(Interval, const uint8_t*)
{
    throw IllegalWriteOperation();
}

template <typename TFile>
void ReadOnlyFile<TFile>::flushFile()
{
    throw IllegalWriteOperation();
}

template <typename TFile>
void ReadOnlyFile<TFile>::truncate(size_t)
{
    throw IllegalWriteOperation();
}

template <typename TFile>
Lock ReadOnlyFile<TFile>::defaultAccess()
{
    return TFile::readAccess();
}

template <typename TFile>
Lock ReadOnlyFile<TFile>::writeAccess()
{
    throw IllegalWriteOperation();
}

template <typename TFile>
CommitLock ReadOnlyFile<TFile>::commitAccess(Lock&&)
{
    throw IllegalWriteOperation();
}



}
