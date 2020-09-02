

#include "FileInterface.h"
#include "LockProtocol.h"
#include "FileSharedMutex.h"
#include "Lock.h"
#include <filesystem>

namespace TxFs
{

class File : public FileInterface
{
public:
    File();
    File(File&&);
    File& operator=(File&&);
    ~File();

    static File create(std::filesystem::path path);
    static File createTemp();
    static File open(std::filesystem::path path, bool readOnly = false);

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
    File(void* handle, bool readOnly);
    void close();

private:
    void* m_handle;
    bool m_readOnly;
    LockProtocol<FileSharedMutex, FileSharedMutex> m_lockProtocol;
};

}
