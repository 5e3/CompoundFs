
#pragma once

#include <gtest/gtest.h>

namespace TxFs
{


template <typename THelper>
struct FileLockingTester : ::testing::Test
{

    THelper m_helper;
    using File = typename THelper::File;
    using FileLock = typename THelper::FileLock;
};

TYPED_TEST_SUITE_P(FileLockingTester);

TYPED_TEST_P(FileLockingTester, throwOnInvalidHandle)
{
    File file = nullptr;  // not initialized
    FileLock fl { file.m_handle, -2, -1 };
    ASSERT_THROW(fl.try_lock(), std::exception);
}

TYPED_TEST_P(FileLockingTester, throwOnEmptyRange)
{
    File file;
    FileLock fl { file.m_handle, 1, 1 };
    ASSERT_THROW(fl.try_lock(), std::exception);
}

TYPED_TEST_P(FileLockingTester, XLockPreventsOtherLocks)
{
    File f;
    FileLock fl { f.m_handle, -2, -1 };
    std::unique_lock lock(fl);

    File f2 = f;
    FileLock fl2 { f2.m_handle, -2, -1 };
    ASSERT_TRUE(!fl2.try_lock_shared());
    ASSERT_TRUE(!fl2.try_lock());
}

TYPED_TEST_P(FileLockingTester, SLockPreventsXLock)
{
    File f;
    FileLock fl { f.m_handle, -2, -1 };
    std::shared_lock lock(fl);

    File f2 = f;
    FileLock fl2 { f2.m_handle, -2, -1 };
    ASSERT_TRUE(!fl2.try_lock());
    ASSERT_TRUE(fl2.try_lock_shared());
}

TYPED_TEST_P(FileLockingTester, differentRegionsAreIndependant)
{
    File f;
    FileLock fl { f.m_handle, 0, 1 };
    std::unique_lock lock(fl);

    File f2 = f;
    FileLock fl2 { f2.m_handle, 1, 2 };
    ASSERT_TRUE(fl2.try_lock());
}

TYPED_TEST_P(FileLockingTester, overlappingRegionsAreNotIndependant)
{
    File f;
    FileLock fl { f.m_handle, 0, 2 };
    std::unique_lock lock(fl);

    File f2 = f;
    FileLock fl2 { f2.m_handle, 1, 3 };
    ASSERT_TRUE(!fl2.try_lock());
}

REGISTER_TYPED_TEST_SUITE_P(FileLockingTester, throwOnInvalidHandle, throwOnEmptyRange, XLockPreventsOtherLocks,
                            SLockPreventsXLock, differentRegionsAreIndependant, overlappingRegionsAreNotIndependant);

}
