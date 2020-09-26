

#include <gtest/gtest.h>
#include "FileInterfaceTester.h"

#include "CompoundFs/MemoryFile.h"

using namespace TxFs;

INSTANTIATE_TYPED_TEST_SUITE_P(Mem, FileInterfaceTester, MemoryFile);


