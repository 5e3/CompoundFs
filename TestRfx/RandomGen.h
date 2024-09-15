
#pragma once

#include <type_traits>
#include <string>
#include <random>

namespace Rfx
{
    template <typename> struct StreamRule;

    struct MySizeType
    {
        size_t m_value;
        operator size_t() const noexcept { return m_value; }
    };

struct RandomGen
{
    using MySizeType = Rfx::MySizeType;

    std::mt19937 m_gen = std::mt19937 { 42 };
    std::uniform_int_distribution<size_t> m_distr = std::uniform_int_distribution<size_t>{ 0, 1000 };

    template <typename T>
    void read(T& value)
        requires std::is_integral_v<T>
    {
        value = m_distr(m_gen)+1;
    }
    template <typename T>
    void read(T& value)
        requires std::is_floating_point_v<T>
    {
        value = (m_distr(m_gen) + 1) / 10.0;
    }
    void read(std::string& value)
    { 
        MySizeType size {};
        read(size);
        value = std::string(size.m_value, char(size.m_value +'A'));
    }
    void read(MySizeType& value) 
    { 
        value = { m_distr(m_gen, std::uniform_int_distribution<size_t>::param_type(0, 10)) };
    }
    template <typename T>
    void read(T& value)
    {
        StreamRule<T>::read(value, *this);
    }

    template <std::ranges::range TRange>
    void readRange(TRange& range)
    {
        for (auto& value: range)
            read(value);
    }
    template <typename T>
    void operator()(T& value)
    {
        read(value);
    }
};

}
