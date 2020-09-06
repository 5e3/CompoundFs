

#include "FileInterface.h"
#include "LockProtocol.h"
#include "FileSharedMutex.h"
#include "Lock.h"
#include <filesystem>

namespace TxFs
{

enum class OpenMode { Create, Open, ReadOnly };

class File : public FileInterface
{
public:
    File();
    File(std::filesystem::path path, OpenMode mode);
    File(File&&);
    File& operator=(File&&);
    ~File();

    void close();

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
    static void* open(std::filesystem::path path, OpenMode mode);
    void writePages(const uint8_t* begin, const uint8_t* end);
    void readPages(uint8_t* begin, uint8_t* end) const;

private:
    void* m_handle;
    bool m_readOnly;
    LockProtocol<FileSharedMutex, FileSharedMutex> m_lockProtocol;
};

///////////////////////////////////////////////////////////////////////////////

class TempFile : public File
{
public:
    TempFile();
    TempFile(TempFile&&);
    TempFile& operator=(TempFile&& other);
    ~TempFile();

private:
    std::filesystem::path m_path;

};

}
