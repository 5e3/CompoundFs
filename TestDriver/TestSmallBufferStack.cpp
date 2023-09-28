


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

    stack.push(1);
    ASSERT_EQ(stack.size(), 1);

    stack.push(2);
    ASSERT_EQ(stack.size(), 2);
}

TEST(SmallBufferStack, BackReturnsTopOfStack)
{
    SmallBufferStack<int, 5> stack;

    stack.push(1);
    stack.push(2);
    ASSERT_EQ(stack.top(), 2);

    for (auto i: { 3, 4, 5, 6 })
        stack.push(i);
    ASSERT_EQ(stack.top(), 6);
}

TEST(SmallBufferStack, PopBackRemovesTopOfStack)
{
    SmallBufferStack<int, 5> stack;

    for (auto i: { 1, 2, 3 })
        stack.push(i);

    ASSERT_EQ(stack.top(), 3);
    stack.pop();
    ASSERT_EQ(stack.top(), 2);
    stack.pop();
    ASSERT_EQ(stack.top(), 1);
    stack.pop();
    ASSERT_TRUE(stack.empty());
}

TEST(SmallBufferStack, PopBackRemovesTopOfStack2)
{
    SmallBufferStack<int, 5> stack;

    for (auto i: { 1, 2, 3, 4, 5, 6 })
        stack.push(i);

    for (auto i: { 6, 5, 4, 3, 2, 1 })
    {
        ASSERT_EQ(stack.top(), i);
        stack.pop();
    }
    ASSERT_TRUE(stack.empty());
}

TEST(SmallBufferStack, EmplacePerfectForwards)
{
    SmallBufferStack<std::shared_ptr<int>, 5> stack;

    auto ptr = std::make_shared<int>(5);
    stack.emplace(std::move(ptr));
    ASSERT_EQ(stack.top().use_count(), 1);
}

TEST(SmallBufferStack, DtorCallsAllElementDtors)
{
    auto ptr = std::make_shared<int>(5);
    {
        SmallBufferStack<std::shared_ptr<int>, 5> stack;
        for (int i = 0; i < 5; i++)
            stack.push(ptr);
        ASSERT_EQ(stack.top().use_count(), 6);
    }
    ASSERT_EQ(ptr.use_count(), 1);

    {
        SmallBufferStack<std::shared_ptr<int>, 5> stack;
        for (int i = 0; i < 6; i++)
            stack.push(ptr);
        ASSERT_EQ(stack.top().use_count(), 7);
    }
    ASSERT_EQ(ptr.use_count(), 1);
}

TEST(SmallBufferStack, PopBackCallsElementDtor)
{
    auto ptr = std::make_shared<int>(5);
    SmallBufferStack<std::shared_ptr<int>, 5> stack;
    for (int i = 0; i < 5; i++)
        stack.push(ptr);
    ASSERT_EQ(stack.top().use_count(), 6);

    stack.pop();
    ASSERT_EQ(stack.top().use_count(), 5);
}

TEST(SmallBufferStack, MovePushBack)
{
    auto ptr = std::make_shared<int>(5);
    SmallBufferStack<std::shared_ptr<int>, 5> stack;
    for (int i = 0; i < 4; i++)
        stack.push(ptr);
    ASSERT_EQ(stack.top().use_count(), 5);

    stack.push(std::move(ptr));
    ASSERT_EQ(stack.top().use_count(), 5);
}