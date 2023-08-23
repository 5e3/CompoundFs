
#pragma once

#include <filesystem>
#include "FileInterface.h"
#include "FileIo.h"

namespace TxFs
{

template<typename TFile>
class TempFile : public TFile
{
public:
    TempFile();
    TempFile(const std::filesystem::path&);
    TempFile(TempFile&&);
    TempFile& operator=(TempFile&& other);
    ~TempFile();

private:
    std::filesystem::path m_path;
};

///////////////////////////////////////////////////////////////////////////////

namespace Private
{
    std::filesystem::path createTempFileName();
}

template <typename TFile>
TempFile<TFile>::TempFile()
    : TempFile(Private::createTempFileName())
{
}

template <typename TFile>
TempFile<TFile>::TempFile(const std::filesystem::path& tmpFileName)
    : TFile(tmpFileName, OpenMode::CreateAlways)
    , m_path(tmpFileName)
{
}

template <typename TFile>
TempFile<TFile>::TempFile(TempFile&& other)
    : TFile(std::move(other))
    , m_path(std::move(other.m_path))
{
    other.m_path = std::filesystem::path();
}

template <typename TFile>
TempFile<TFile>::~TempFile()
{
    TFile::close();
    if (!m_path.empty())
        std::filesystem::remove(m_path);
}

template <typename TFile>
TempFile<TFile>& TempFile<TFile>::operator=(TempFile&& other)
{
    TFile::close();
    if (!m_path.empty())
        std::filesystem::remove(m_path);
    TFile::operator=(std::move(other));
    m_path = std::move(other.m_path);
    other.m_path = std::filesystem::path();
    return *this;
}

}


