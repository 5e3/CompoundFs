
#pragma once

#include <variant>
#include <array>
#include <stdexcept>
#include <iterator>

namespace
{

template <typename... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

}

namespace TxFs
{

//////////////////////////////////////////////////////////////////////////

template <typename T, size_t SIZE>
class FixedStack final
{
    std::array<T, SIZE> m_stack;
    T* m_end;

public:
    FixedStack()
        : m_end(m_stack.data())
    {
    }

    void push_back(const T& value)
    {
        *m_end = value;
        ++m_end;
    }

    template<class... Args>
    void emplace_back(Args&&... args)
    {
        *m_end = T(std::forward<Args>(args)...);
        ++m_end;
    }

    T& back() { return *(m_end - 1); }
    const T& back() const { return *(m_end - 1); }

    T* end() { return m_end; }
    const T* end() const { return m_end; }

    T* data() { return m_stack.data(); }

    void pop_back() { --m_end; }

    size_t size() const 
    { 
        return m_end - m_stack.data(); 
    }

    bool empty() const 
    { 
        return size() == 0; 
    }

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
            stack.reserve(fixedStack.size() * 2);
            stack.assign(fixedStack.data(), fixedStack.data() + SIZE);
            m_stack = std::move(stack);
        }
    }

public:
    void push_back(const T& value)
    {
        switchToVectorOnOverflow();
        std::visit([value](auto& stack) { stack.push_back(value);}, m_stack);
    }

    bool empty() const
    {
        return std::visit([](auto&& stack) { return stack.empty(); }, m_stack);
    }

    size_t size() const
    {
        return std::visit([](auto&& stack) { return stack.size(); }, m_stack);
    }
    
    T& back()
    {
        return std::visit([](auto&& stack) -> T& { return stack.back(); }, m_stack);
    }

    const T& back() const
    {
        return std::visit([](auto&& stack) -> const T& { return stack.back(); }, m_stack);
    }

    void pop_back()
    {
        std::visit([](auto&& stack) { stack.pop_back(); }, m_stack);
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

    template<class... Args>
    void emplace_back(Args&&... args)
    {
        switchToVectorOnOverflow();
        std::visit([&args...](auto&& stack) { stack.emplace_back(std::forward<Args>(args)...); }, m_stack);
    }
};
}
