
#include <gtest/gtest.h>

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
