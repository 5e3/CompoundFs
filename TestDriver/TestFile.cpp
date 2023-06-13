

#include <gtest/gtest.h>
#include "DiskFileTester.h"
#include "FileInterfaceTester.h"

#include "CompoundFs/File.h"
#include "CompoundFs/TempFile.h"
#include "CompoundFs/ByteString.h"

#pragma warning(disable : 4996) // disable "'tmpnam': This function or variable may be unsafe."

using namespace TxFs;


TEST(File, canWriteOnWriteLockedFile)
{
    auto wfile = TempFile<File>();
    auto rfile = File(wfile.getFileName(), OpenMode::ReadOnly);
    auto rlock = rfile.defaultAccess();
    auto wlock = wfile.defaultAccess();
    wfile.newInterval(5);
    ByteStringView out("0123456789");
    ASSERT_NO_THROW(wfile.writePage(1, 0, out.data(), out.end()));
}

TEST(File, canReadFromWriteLockedFile)
{
    auto wfile = TempFile<File>();
    auto rfile = File(wfile.getFileName(), OpenMode::ReadOnly);
    auto rlock = rfile.defaultAccess();
    auto wlock = wfile.defaultAccess();
    wfile.newInterval(5);
    ByteStringView out("0123456789");
    ASSERT_NO_THROW(wfile.writePage(1, 0, out.data(), out.end()));
    uint8_t buf[11];
    rfile.readPage(1, 0, buf, buf + sizeof(buf));
}

INSTANTIATE_TYPED_TEST_SUITE_P(WinFile, DiskFileTester, File);

INSTANTIATE_TYPED_TEST_SUITE_P(WinFile, FileInterfaceTester, TempFile<File>);
