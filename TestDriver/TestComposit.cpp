
#include <gtest/gtest.h>
#include "FileSystemHelper.h"

#include "../CompoundFs/Composit.h"
#include "../CompoundFs/MemoryFile.h"
#include "../CompoundFs/WrappedFile.h"
#include "../CompoundFs/Path.h"

using namespace TxFs;

TEST(Composit, EmptyFileGetsInitialzed)
{
    std::shared_ptr<FileInterface> file = std::make_shared<MemoryFile>();
    Composit::open<WrappedFile>(file);
    ASSERT_GT(file->currentSize(), 0);
}

TEST(Composit, canReopenFile)
{
    std::shared_ptr<FileInterface> file = std::make_shared<MemoryFile>();
    {
        auto fsys = Composit::open<WrappedFile>(file);
        fsys.addAttribute("test", "test");
        fsys.commit();
    }

    auto fsys = Composit::open<WrappedFile>(file);
    ASSERT_EQ(fsys.getAttribute("test")->toValue<std::string>(), "test");
}

TEST(Composit, openDoesRollback)
{
    std::shared_ptr<FileInterface> file = std::make_shared<MemoryFile>();
    {
        auto fsys = Composit::open<WrappedFile>(file);
        auto handle = *fsys.createFile("file");
        std::string data(5000, 'X');
        fsys.write(handle, data.data(), data.size());
        // no commit()
    }

    auto size = file->currentSize();
    auto fsys = Composit::open<WrappedFile>(file);
    ASSERT_LT(file->currentSize(), size);
}

struct CompositTester : ::testing::Test
{
    std::shared_ptr<FileInterface> m_file;
    FileSystemHelper m_helper;

    CompositTester()
        : m_file(std::make_shared<MemoryFile>())
    {
        auto fsys = Composit::open<WrappedFile>(m_file);
        m_helper.fillFileSystem(fsys);
        fsys.commit();
    }
};

TEST_F(CompositTester, selfTest)
{
    auto fsys = Composit::open<WrappedFile>(m_file);
    m_helper.checkFileSystem(fsys);
}

TEST_F(CompositTester, openDoesRollback)
{
    {
        auto fsys = Composit::open<WrappedFile>(m_file);
        auto out = makeFile(fsys, "testFile", 'x');
        auto in = readFile(fsys, "testFile");
        ASSERT_EQ(out, in);
    }

    auto fsys = Composit::open<WrappedFile>(m_file);
    ASSERT_FALSE(fsys.readFile("testFile"));
}

TEST_F(CompositTester, commitedDeletedSpaceGetsReused)
{
    {
        auto fsys = Composit::open<WrappedFile>(m_file);
        fsys.remove("test");
        fsys.commit();
    }

    auto fsys = Composit::open<WrappedFile>(m_file);
    auto size = m_file->currentSize();
    makeFile(fsys, "testFile");
    ASSERT_EQ(size, m_file->currentSize());
}

TEST_F(CompositTester, maxFolderIdGetsWrittenOnCommit)
{
    auto fsys = Composit::open<WrappedFile>(m_file);
    ASSERT_TRUE(fsys.makeSubFolder("test2"));
    ASSERT_TRUE(fsys.remove("test2")); // might delete contents of "test"
    m_helper.checkFileSystem(fsys);
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

TEST_F(CompositTester, RollbackFromCrashedCommit)
{
    {
	    auto fsys = Composit::open<CrashCommitFile>(m_file);
	    fsys.remove("test");
	    ASSERT_FALSE(fsys.getAttribute("test/attribute"));
	    ASSERT_THROW(fsys.commit(), CrashCommitFile::Exception);
    }

    auto fsys = Composit::open<WrappedFile>(m_file);
    ASSERT_TRUE(fsys.getAttribute("test/attribute"));
    m_helper.checkFileSystem(fsys);
}
