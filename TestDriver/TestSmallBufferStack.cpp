


#include <gtest/gtest.h>
#include "CompoundFs/SmallBufferStack.h"
//#include <algorithm>
//#include <random>

using namespace TxFs;

TEST(SmallBufferStack, DefaultCtorMakesEmptyStack)
{
    SmallBufferStack<int, 5> stack;
    ASSERT_TRUE(stack.empty());
}

TEST(SmallBufferStack, PushBackAddsToTheStack)
{
    SmallBufferStack<int, 5> stack;

    stack.push_back(1);
    ASSERT_EQ(stack.size(), 1);

    stack.push_back(2);
    ASSERT_EQ(stack.size(), 2);
}

TEST(SmallBufferStack, BackReturnsTopOfStack)
{
    SmallBufferStack<int, 5> stack;

    stack.push_back(1);
    stack.push_back(2);
    ASSERT_EQ(stack.back(), 2);

    for (auto i: { 3, 4, 5, 6 })
        stack.push_back(i);
    ASSERT_EQ(stack.back(), 6);
}

TEST(SmallBufferStack, PopBackRemovesTopOfStack)
{
    SmallBufferStack<int, 5> stack;

    for (auto i: { 1, 2, 3 })
        stack.push_back(i);

    ASSERT_EQ(stack.back(), 3);
    stack.pop_back();
    ASSERT_EQ(stack.back(), 2);
    stack.pop_back();
    ASSERT_EQ(stack.back(), 1);
    stack.pop_back();
    ASSERT_TRUE(stack.empty());
}

TEST(SmallBufferStack, PopBackRemovesTopOfStack2)
{
    SmallBufferStack<int, 5> stack;

    for (auto i: { 1, 2, 3, 4, 5, 6 })
        stack.push_back(i);

    for (auto i: { 6, 5, 4, 3, 2, 1 })
    {
        ASSERT_EQ(stack.back(), i);
        stack.pop_back();
    }
    ASSERT_TRUE(stack.empty());
}

TEST(SmallBufferStack, EmplacePerfectForwards)
{
    SmallBufferStack<std::shared_ptr<int>, 5> stack;

    auto ptr = std::make_shared<int>(5);
    stack.emplace_back(std::move(ptr));
    ASSERT_EQ(stack.back().use_count(), 1);
}
