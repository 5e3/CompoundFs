

#include <gtest/gtest.h>
#include "DiskFileTester.h"

#include "CompoundFs/File.h"
#include "CompoundFs/ByteString.h"

#pragma warning(disable : 4996) // disable "'tmpnam': This function or variable may be unsafe."

using namespace TxFs;


TEST(File, canWriteOnWriteLockedFile)
{
    auto wfile = TempFile();
    auto rfile = File(wfile.getFileName(), OpenMode::ReadOnly);
    auto rlock = rfile.defaultAccess();
    auto wlock = wfile.defaultAccess();
    wfile.newInterval(5);
    ByteStringView out("0123456789");
    wfile.writePage(1, 0, out.data(), out.end());
    ASSERT_NO_THROW(wfile.writePage(1, 0, out.data(), out.end()));
}


INSTANTIATE_TYPED_TEST_SUITE_P(_, DiskFileTester, File);

