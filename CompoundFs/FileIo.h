

#include "FileInterface.h"
#include "Lock.h"
#include <filesystem>

namespace TxFs
{

class FileIo : public FileInterface
{
public:
    FileIo();
    FileIo(FileIo&&);
    FileIo& operator=(FileIo&&);
    ~FileIo();

    static FileIo create(std::filesystem::path path);
    static FileIo open(std::filesystem::path path, bool readOnly = false);

    Interval newInterval(size_t maxPages) override;
    const uint8_t* writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end) override;
    const uint8_t* writePages(Interval iv, const uint8_t* page) override;
    uint8_t* readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const override;
    uint8_t* readPages(Interval iv, uint8_t* page) const override;
    size_t currentSize() const override; // file size in number of pages
    void flushFile() override;
    void truncate(size_t numberOfPages) override;

    Lock defaultAccess() override;
    Lock readAccess() override;
    Lock writeAccess() override;
    CommitLock commitAccess(Lock&& writeLock) override;

    std::filesystem::path getFileName() const;

private:
    FileIo(void* handle, bool readOnly);
    void close();

private:
    void* m_handle;
    bool m_readOnly;
};

}
