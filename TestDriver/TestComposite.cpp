
#include <gtest/gtest.h>
#include "FileSystemUtility.h"

#include "CompoundFs/Composite.h"
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/WrappedFile.h"
#include "CompoundFs/Path.h"
#include "CompoundFs/RollbackHandler.h"
#include "CompoundFs/FileIo.h"
#include "CompoundFs/WindowsFile.h"
#include "CompoundFs/TempFile.h"

#include <fstream>

using namespace TxFs;

TEST(Composite, EmptyFileGetsInitialzed)
{
    std::shared_ptr<FileInterface> file = std::make_shared<MemoryFile>();
    Composite::open<WrappedFile>(file);
    ASSERT_GT(file->fileSizeInPages(), 0U);
}

TEST(Composite, canReopenFile)
{
    std::shared_ptr<FileInterface> file = std::make_shared<MemoryFile>();
    {
        auto fsys = Composite::open<WrappedFile>(file);
        fsys.addAttribute("test", "test");
        fsys.commit();
    }

    auto fsys = Composite::open<WrappedFile>(file);
    ASSERT_EQ(fsys.getAttribute("test")->get<std::string>(), "test");
}

TEST(Composite, openDoesRollback)
{
    std::shared_ptr<FileInterface> file = std::make_shared<MemoryFile>();
    {
        auto fsys = Composite::open<WrappedFile>(file);
        auto handle = *fsys.createFile("file");
        std::string data(5000, 'X');
        fsys.write(handle, data.data(), data.size());
        // no commit()
    }

    auto size = file->fileSizeInPages();
    auto fsys = Composite::open<WrappedFile>(file);
    ASSERT_LT(file->fileSizeInPages(), size);
}

TEST(Composite, openNonTxFsFileThrows)
{
    std::shared_ptr<FileInterface> file = std::make_shared<MemoryFile>();
    {
        ByteStringView data("12345");
        file->newInterval(1);
        file->writePage(0, 0, data.data(), data.end());
    }

    ASSERT_THROW(Composite::open<WrappedFile>(file), std::exception);
}

struct CompositeTester : ::testing::Test
{
    using MemoryFile = LockedMemoryFile<DebugSharedLock, DebugSharedLock>;
    DebugSharedLock m_gateLock;
    DebugSharedLock m_sharedLock;
    DebugSharedLock m_writerLock;
    std::shared_ptr<FileInterface> m_file;
    FileSystemUtility m_helper;

    CompositeTester()
    {
        auto gate = m_gateLock;
        auto shared = m_sharedLock;
        auto writer = m_writerLock;
        auto lp = std::make_unique<MemoryFile::TLockProtocol>(std::move(gate), std::move(shared), std::move(writer));
        m_file = std::make_shared<MemoryFile>(std::move(lp));
        auto fsys = Composite::open<WrappedFile>(m_file);
        m_helper.fillFileSystem(fsys);
        fsys.commit();
    }
};

TEST_F(CompositeTester, selfTest)
{
    auto fsys = Composite::open<WrappedFile>(m_file);
    m_helper.checkFileSystem(fsys);
}

TEST_F(CompositeTester, openDoesRollback)
{
    {
        auto fsys = Composite::open<WrappedFile>(m_file);
        auto out = makeFile(fsys, "testFile", 'x');
        auto in = readFile(fsys, "testFile");
        ASSERT_EQ(out, in);
    }

    auto fsys = Composite::open<WrappedFile>(m_file);
    ASSERT_FALSE(fsys.readFile("testFile"));
}

TEST_F(CompositeTester, commitedDeletedSpaceGetsReused)
{
    {
        auto fsys = Composite::open<WrappedFile>(m_file);
        fsys.remove("test");
        fsys.commit();
    }

    auto fsys = Composite::open<WrappedFile>(m_file);
    auto size = m_file->fileSizeInPages();
    makeFile(fsys, "testFile");
    ASSERT_EQ(size, m_file->fileSizeInPages());
}

TEST_F(CompositeTester, maxFolderIdGetsWrittenOnCommit)
{
    auto fsys = Composite::open<WrappedFile>(m_file);
    ASSERT_TRUE(fsys.makeSubFolder("test2"));
    ASSERT_TRUE(fsys.remove("test2")); // might delete contents of "test"
    m_helper.checkFileSystem(fsys);
}

TEST_F(CompositeTester, findAllEntriesAfterRootPageOverflow)
{
    uint64_t i = 0;
    {
        auto fsys = Composite::open<WrappedFile>(m_file);

        auto csize = m_file->fileSizeInPages();
        while (csize == m_file->fileSizeInPages())
        {
            auto data = std::to_string(i++);
            fsys.addAttribute(Path(data), i);
        }
        fsys.commit();
    }

    auto fsys = Composite::open<WrappedFile>(m_file);
    for (uint64_t j = 0; j < i; j++)
    {
        auto data = std::to_string(j);
        ASSERT_TRUE(fsys.getAttribute(Path(data)));
    }
}

TEST_F(CompositeTester, findAllEntriesAfterRootPageUnderflow)
{
    uint64_t i = 0;
    {
        auto fsys = Composite::open<WrappedFile>(m_file);
        fsys.remove("test");
        fsys.commit();

        // overflow btree
        auto csize = m_file->fileSizeInPages();
        while (csize == m_file->fileSizeInPages())
        {
            auto data = std::to_string(i++);
            fsys.addAttribute(Path(data), i);
        }
        fsys.commit();
    }
    {    
        auto fsys = Composite::open<WrappedFile>(m_file);
        for (uint64_t j = 0; j < i; j++)
        {
            auto data = std::to_string(j);
            ASSERT_EQ(fsys.remove(Path(data)), 1);
        }
        fsys.commit();
        m_helper.fillFileSystem(fsys);
        fsys.commit();
    }

    auto fsys = Composite::open<WrappedFile>(m_file);
    m_helper.checkFileSystem(fsys);
}

TEST_F(CompositeTester, treeSpaceGetsReused)
{
    uint64_t i = 0;
    size_t csize = 0;
    {
        auto fsys = Composite::open<WrappedFile>(m_file);
        fsys.remove("test");
        fsys.commit();

        csize = m_file->fileSizeInPages();
        while (csize == m_file->fileSizeInPages())
        {
            auto data = std::to_string(i++);
            fsys.addAttribute(Path(data), data);
        }
        fsys.commit();
        csize = m_file->fileSizeInPages();
        for (uint64_t j = 0; j < i; j++)
            ASSERT_EQ(fsys.remove(Path(std::to_string(j))), 1);
        fsys.commit();
    }

    auto fsys = Composite::open<WrappedFile>(m_file);
    for (uint64_t j = 0; j < i; j++)
    {
        auto data = std::to_string(j++);
        fsys.addAttribute(Path(data), data);
    }
    ASSERT_EQ(m_file->fileSizeInPages(), csize);
}

struct CrashCommitFile : WrappedFile
{
    CrashCommitFile(std::shared_ptr<FileInterface> file)
        : WrappedFile(file)
    {}

    struct Exception : std::runtime_error
    {
        Exception()
            : std::runtime_error("")
        {}
    };

    void truncate(size_t numberOfPages) override
    {
        if (fileSizeInPages() != numberOfPages)
            throw Exception();
        WrappedFile::truncate(numberOfPages);
    }
};

TEST_F(CompositeTester, CrashCommitFileProvidesReplacedDirtyPages)
{
    {
        auto fsys = Composite::open<CrashCommitFile>(m_file);
        fsys.remove("test");
        ASSERT_THROW(fsys.commit(), CrashCommitFile::Exception);
    }

    CacheManager cm{std::make_unique<WrappedFile>(m_file)};
    auto rollbackHandler = cm.getRollbackHandler();
    auto logs = rollbackHandler.readLogs();
    ASSERT_FALSE(logs.empty());
    for (auto [orig, cpy]: logs)
        ASSERT_FALSE(TxFs::isEqualPage(m_file.get(), orig, cpy));
}

TEST_F(CompositeTester, RollbackFromCrashedCommit)
{
    {
        auto fsys = Composite::open<CrashCommitFile>(m_file);
        fsys.remove("test");
        ASSERT_FALSE(fsys.getAttribute("test/attribute"));
        ASSERT_THROW(fsys.commit(), CrashCommitFile::Exception);
    }

    auto fsys = Composite::open<WrappedFile>(m_file);
    ASSERT_TRUE(fsys.getAttribute("test/attribute"));
    m_helper.checkFileSystem(fsys);
}

TEST_F(CompositeTester, ReadOnlyRollbackFromCrashedCommit)
{
    {
        auto fsys = Composite::open<CrashCommitFile>(m_file);
        fsys.remove("test");
        ASSERT_FALSE(fsys.getAttribute("test/attribute"));
        ASSERT_THROW(fsys.commit(), CrashCommitFile::Exception);
    }

    auto fsys = Composite::openReadOnly<WrappedFile>(m_file);
    ASSERT_TRUE(fsys.getAttribute("test/attribute"));
    m_helper.checkFileSystem(fsys);
}

TEST_F(CompositeTester, ReadOnlyCommitDoesNotThrow)
{
    auto fsys = Composite::openReadOnly<WrappedFile>(m_file);
    fsys.fileSize("folder/file.file");
    ASSERT_NO_THROW(fsys.commit());
}
