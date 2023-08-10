
#pragma once

#include <filesystem>

#include "FileInterface.h"

namespace TxFs
{

class PosixFile : public FileInterface
{
public:
    PosixFile();
    PosixFile(std::filesystem::path path, OpenMode mode);
    PosixFile(PosixFile&&) noexcept;
    PosixFile& operator=(PosixFile&&) noexcept;
    ~PosixFile();

    void close();

    Interval newInterval(size_t maxPages) override;
    const uint8_t* writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end) override;
    const uint8_t* writePages(Interval iv, const uint8_t* page) override;
    uint8_t* readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const override;
    uint8_t* readPages(Interval iv, uint8_t* page) const override;
    size_t fileSizeInPages() const override;
    void flushFile() override;
    void truncate(size_t numberOfPages) override;
    Lock defaultAccess() override;
    Lock readAccess() override;
    Lock writeAccess() override;
    CommitLock commitAccess(Lock&& writeLock) override;
    
private:
    PosixFile(int file, bool readOnly);
    static int open(std::filesystem::path path, OpenMode mode);
    void writePagesInBlocks(const uint8_t* begin, const uint8_t* end);
    size_t readPagesInBlocks(uint8_t* begin, uint8_t* end) const;


private:
    int m_file;
    bool m_readOnly;
};
}