
#pragma once

#include <variant>
#include <memory>
#include <iterator>

namespace TxFs
{

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t SIZE>
class FixedStack final
{
    alignas(T) char m_buffer[sizeof(T) * SIZE];
    T* m_end;

public:
    FixedStack() noexcept
        : m_end(data())
    {
    }

    FixedStack(const FixedStack& other)
    {
        clear();
        for (const auto& e: other)
            push_back(e);
    }

    FixedStack(FixedStack&& other) noexcept
    {
        clear();
        for (auto it = std::make_move_iterator(other.begin()); it != std::make_move_iterator(other.end()); ++it)
            emplace(std::move(*it));
    }

    FixedStack& operator=(const FixedStack& other)
    {
        if (&other == this)
            return *this;
        clear();
        for (const auto& e: other)
            push_back(e);
        return *this;
    }

    FixedStack& operator=(FixedStack&& other) noexcept
    {
        if (&other == this)
            return *this;
        clear();
        for (auto it = std::make_move_iterator(other.begin()); it != std::make_move_iterator(other.end()); ++it)
            emplace(std::move(*it));
        return *this;
    }

    ~FixedStack() { std::destroy(data(), m_end); }

    void clear() noexcept
    {
        std::destroy(begin(), end());
        m_end = data();
    }

    void push_back(const T& value) { emplace_back(T(value)); }
    void push_back(T&& value) noexcept { emplace_back(std::move(value)); }

    template <class... Args>
    void emplace_back(Args&&... args) noexcept
    {
        new (m_end) T(std::forward<Args>(args)...);
        ++m_end;
    }

    T& back() noexcept { return *(m_end - 1); }
    const T& back() const noexcept { return *(m_end - 1); }

    T* end() noexcept { return m_end; }
    const T* end() const noexcept { return m_end; }

    T* data() noexcept { return reinterpret_cast<T*>(m_buffer); }
    const T* data() const noexcept { return reinterpret_cast<const T*>(m_buffer); }
    T* begin() noexcept { return data(); }
    const T* begin() const noexcept { return data(); }

    void pop_back() noexcept
    { 
        std::destroy(m_end-1, m_end);
        --m_end;
    }

    size_t size() const noexcept { return m_end - data(); }

    bool empty() const noexcept { return size() == 0; }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T, size_t SIZE>
class SmallBufferStack final
{
    std::variant<FixedStack<T, SIZE>, std::vector<T>> m_stack;

    void switchToVectorOnOverflow()
    {
        if (m_stack.index() == 0 && std::get<0>(m_stack).size() == SIZE)
        {
            auto& fixedStack = std::get<0>(m_stack);
            std::vector<T> stack;
            stack.reserve(SIZE * 2);
            stack.assign(std::make_move_iterator(fixedStack.data()), std::make_move_iterator(fixedStack.data() + SIZE));
            m_stack = std::move(stack);
        }
    }

public:
    void push(const T& value)
    {
        switchToVectorOnOverflow();
        std::visit([value](auto& stack) { stack.push_back(value); }, m_stack);
    }

    void push(T&& value)
    {
        switchToVectorOnOverflow();
        std::visit([&value](auto& stack) { stack.push_back(std::move(value)); }, m_stack);
    }

    template <class... Args>
    void emplace(Args&&... args)
    {
        switchToVectorOnOverflow();
        std::visit([&args...](auto&& stack) { stack.emplace_back(std::forward<Args>(args)...); }, m_stack);
    }

    void pop()
    {
        std::visit([](auto&& stack) { stack.pop_back(); }, m_stack);
    }

    bool empty() const
    {
        return std::visit([](auto&& stack) { return stack.empty(); }, m_stack);
    }

    size_t size() const
    {
        return std::visit([](auto&& stack) { return stack.size(); }, m_stack);
    }

    T& top()
    {
        return std::visit([](auto&& stack) -> T& { return stack.back(); }, m_stack);
    }

    const T& top() const
    {
        return std::visit([](auto&& stack) -> const T& { return stack.back(); }, m_stack);
    }

    T* end()
    {
        return std::visit([](auto&& stack) { return &(*stack.end()); }, m_stack);
    }

    const T* end() const
    {
        return std::visit([](auto&& stack) { return &(*stack.end()); }, m_stack);
    }

    void reserve(size_t) {}

};
}
