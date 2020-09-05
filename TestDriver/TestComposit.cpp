
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


