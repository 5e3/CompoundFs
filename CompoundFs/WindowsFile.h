

#include "FileInterface.h"
#include "LockProtocol.h"
#include "FileLockWindows.h"
#include <filesystem>

namespace TxFs
{

 //////////////////////////////////////////////////////////////////////////////
/// Implementation of a physical file for Windows. No copying but moving. Expect
/// std::exception derived classes on error.
class WindowsFile : public FileInterface
{
public:
    WindowsFile();
    WindowsFile(std::filesystem::path path, OpenMode mode);
    WindowsFile(WindowsFile&&) noexcept;
    WindowsFile& operator=(WindowsFile&&) noexcept;
    ~WindowsFile();

    void close();

    Interval newInterval(size_t maxPages) override;
    const uint8_t* writePage(PageIndex id, size_t pageOffset, const uint8_t* begin, const uint8_t* end) override;
    const uint8_t* writePages(Interval iv, const uint8_t* page) override;
    uint8_t* readPage(PageIndex id, size_t pageOffset, uint8_t* begin, uint8_t* end) const override;
    uint8_t* readPages(Interval iv, uint8_t* page) const override;
    size_t fileSizeInPages() const override; // file size in number of pages
    void flushFile() override;
    void truncate(size_t numberOfPages) override;

    Lock defaultAccess() override;
    Lock readAccess() override;
    Lock writeAccess() override;
    CommitLock commitAccess(Lock&& writeLock) override;

    std::filesystem::path getFileName() const;

private:
    WindowsFile(void* handle, bool readOnly);
    static void* open(std::filesystem::path path, OpenMode mode);
    void writePagesInBlocks(const uint8_t* begin, const uint8_t* end);
    size_t readPagesInBlocks(uint8_t* begin, uint8_t* end) const;

private:
    void* m_handle;
    bool m_readOnly;

protected:
    LockProtocol<FileLockWindows, FileLockWindows> m_lockProtocol;
};


}
