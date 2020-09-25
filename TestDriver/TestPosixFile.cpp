
#include <gtest/gtest.h>
#include "DiskFileTester.h"
#include "CompoundFs/PosixFile.h"

#pragma warning(disable : 4996) // disable "'tmpnam': This function or variable may be unsafe."

using namespace TxFs;

INSTANTIATE_TYPED_TEST_SUITE_P(Posix, DiskFileTester, PosixFile);

