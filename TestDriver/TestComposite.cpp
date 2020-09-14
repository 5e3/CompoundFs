
#include <gtest/gtest.h>
#include "FileSystemHelper.h"

#include "CompoundFs/Composite.h"
#include "CompoundFs/MemoryFile.h"
#include "CompoundFs/WrappedFile.h"
#include "CompoundFs/Path.h"
#include "CompoundFs/RollbackHandler.h"

using namespace TxFs;

TEST(Composite, EmptyFileGetsInitialzed)
{
    std::shared_ptr<FileInterface> file = std::make_shared<MemoryFile>();
    Composite::open<WrappedFile>(file);
    ASSERT_GT(file->currentSize(), 0U);
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
    ASSERT_EQ(fsys.getAttribute("test")->toValue<std::string>(), "test");
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

    auto size = file->currentSize();
    auto fsys = Composite::open<WrappedFile>(file);
    ASSERT_LT(file->currentSize(), size);
}

struct CompositeTester : ::testing::Test
{
    std::shared_ptr<FileInterface> m_file;
    FileSystemHelper m_helper;

    CompositeTester()
        : m_file(std::make_shared<MemoryFile>())
    {
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
    auto size = m_file->currentSize();
    makeFile(fsys, "testFile");
    ASSERT_EQ(size, m_file->currentSize());
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

        auto csize = m_file->currentSize();
        while (csize == m_file->currentSize())
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
        auto csize = m_file->currentSize();
        while (csize == m_file->currentSize())
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

        csize = m_file->currentSize();
        while (csize == m_file->currentSize())
        {
            auto data = std::to_string(i++);
            fsys.addAttribute(Path(data), data);
        }
        fsys.commit();
        csize = m_file->currentSize();
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
    ASSERT_EQ(m_file->currentSize(), csize);
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
        if (currentSize() != numberOfPages)
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
    //m_file.
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
