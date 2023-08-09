
#pragma once

#include <gtest/gtest.h>
#include <mutex>
#include <shared_mutex>

namespace TxFs
{


template <typename THelper>
struct FileLockingTester : ::testing::Test
{
};

TYPED_TEST_SUITE_P(FileLockingTester);

TYPED_TEST_P(FileLockingTester, throwOnInvalidHandle)
{
    typename TypeParam::File file = nullptr;  // not initialized
    typename TypeParam::FileLock fl { file.m_handle, 0, 1 };
    ASSERT_THROW(fl.try_lock(), std::exception);
}

TYPED_TEST_P(FileLockingTester, throwOnEmptyRange)
{
    typename TypeParam::File file;
    typename TypeParam::FileLock fl { file.m_handle, 1, 1 };
    ASSERT_THROW(fl.try_lock(), std::exception);
}

TYPED_TEST_P(FileLockingTester, XLockPreventsOtherLocks)
{
    typename TypeParam::File f;
    typename TypeParam::FileLock fl { f.m_handle, 0, 1 };
    std::unique_lock lock(fl);

    typename TypeParam::File f2 = f;
    typename TypeParam::FileLock fl2 { f2.m_handle, 0, 1 };
    ASSERT_TRUE(!fl2.try_lock_shared());
    ASSERT_TRUE(!fl2.try_lock());
}

TYPED_TEST_P(FileLockingTester, SLockPreventsXLock)
{
    typename TypeParam::File f;
    typename TypeParam::FileLock fl { f.m_handle, 0, 1 };
    std::shared_lock lock(fl);

    typename TypeParam::File f2 = f;
    typename TypeParam::FileLock fl2 { f2.m_handle, 0, 1 };
    ASSERT_TRUE(!fl2.try_lock());               
    ASSERT_TRUE(fl2.try_lock_shared());
}

TYPED_TEST_P(FileLockingTester, differentRegionsAreIndependant)
{
    typename TypeParam::File f;
    typename TypeParam::FileLock fl { f.m_handle, 0, 1 };
    std::unique_lock lock(fl);

    typename TypeParam::File f2 = f;
    typename TypeParam::FileLock fl2 { f2.m_handle, 1, 2 };
    ASSERT_TRUE(fl2.try_lock());
}

TYPED_TEST_P(FileLockingTester, overlappingRegionsAreNotIndependant)
{
    typename TypeParam::File f;
    typename TypeParam::FileLock fl { f.m_handle, 0, 2 };
    std::unique_lock lock(fl);

    typename TypeParam::File f2 = f;
    typename TypeParam::FileLock fl2 { f2.m_handle, 1, 3 };
    ASSERT_TRUE(!fl2.try_lock());
}

REGISTER_TYPED_TEST_SUITE_P(FileLockingTester, throwOnInvalidHandle, throwOnEmptyRange, XLockPreventsOtherLocks,
                            SLockPreventsXLock, differentRegionsAreIndependant, overlappingRegionsAreNotIndependant);

}
