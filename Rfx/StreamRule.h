
#pragma once

#include <ranges>
#include "TypeUtils.h"
#include <tuple>
#include <variant>
#include <optional>
#include <array>


namespace Rfx
{


///////////////////////////////////////////////////////////////////////////////
// StreamRule defines read()/write() methods for certain types. 
// primery template is left undefined
template <typename T>
struct StreamRule;

///////////////////////////////////////////////////////////////////////////////
// restricted StreamRule for user defined types. They require a freestanding function
// called 
// template<typename TVisitor> void forEachMember(MyType& val, TVisitor&& visitor);
template <typename T>
    requires HasForEachMember_v<T>
struct StreamRule<T>
{
    static constexpr bool Versioned = true;

    template <typename TVisitor>
    static void write(const T& value, TVisitor&& visitor)
    {
        forEachMember((T&) value, std::forward<TVisitor>(visitor));
    }

    template <typename TVisitor>
    static void read(T& value, TVisitor&& visitor)
    {
        forEachMember(value, std::forward<TVisitor>(visitor));
    }
};

///////////////////////////////////////////////////////////////////////////////
// StreamRule for STL-like containers (std::vector<>, std::map<> but not std::array[]).
template <DynamicContainer TCont>
struct StreamRule<TCont>
{
    template <typename TStream>
    static void write(const TCont& cont, TStream&& stream)
    {
        SizeType_t<TStream> size { cont.size() };
        stream.write(size);
        stream.writeRange(cont);
    }

    template <typename TStream>
    static void read(TCont& cont, TStream&& stream)
        requires CanResize_v<TCont>
    {
        SizeType_t<TStream> size {};
        stream.read(size);
        cont.resize(size);
        stream.readRange(cont);
    }

    template <typename TStream>
    static void read(TCont& cont, TStream&& stream)
        requires CanInsert_v<TCont>
    {
        cont.clear();
        SizeType_t<TStream> size {};
        stream.read(size);
        for (size_t i = 0; i < size; ++i)
        {
            typename TCont::value_type value {};
            stream.read(value);
            cont.insert(std::move(value));
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
// StreamRule for TupleLike (std::pair, std::tuple and std::array)
// Note that a std::tuple<> is versioned but a std::pair<> is not.
template <TupleLike T>
struct StreamRule<T>
{
    static constexpr bool Versioned = IsSpecialization_v<T, std::tuple>;

    template<typename U>
    static U& ccast(const U& val) { return const_cast<U&>(val);} 

    template <typename TStream>
    static void write(const T& val, TStream&& stream)
    {
        std::apply([&stream](const auto&... telem) { ((stream.write(telem)), ...); }, val);
    }

    template <typename TStream>
    static void read(T& val, TStream&& stream)
    {
        std::apply([&stream](auto&... telem) { ((stream.read(ccast(telem))), ...); }, val);
    }
};


///////////////////////////////////////////////////////////////////////////////
template <typename... Ts>
struct StreamRule<std::variant<Ts...>>
{
    static constexpr bool Versioned = true;

    static std::variant<Ts...> defaultCreateVariantByIndex(size_t index)
    {
            static constexpr std::array<std::variant<Ts...>(*)(), sizeof...(Ts)> defaultCreator
                = { []() -> std::variant<Ts...> { return Ts {}; }...};
            return defaultCreator.at(index)();
    }

    template<typename TStream>
    static void write(const std::variant<Ts...>& var, TStream&& stream)
    {
        SizeType_t<TStream> index { var.index() };
        stream.write(index);
        std::visit([&stream ](const auto& value) { stream.write(value); }, var);
    }

    template<typename TStream>
    static void read(std::variant<Ts...>& var, TStream&& stream)
    {
        SizeType_t<TStream> index {};
        stream.read(index);
        if (index < sizeof...(Ts))
        {
            var = defaultCreateVariantByIndex(index);
            std::visit([&stream](auto& value) { stream.read(value); }, var);
        }
        else
            var = std::variant<Ts...> {};
    }
};

///////////////////////////////////////////////////////////////////////////////
// StreamRule for fixed sized arrays (T[N] or std::array<T,N>).
template <FixedSizeArray T>
struct StreamRule<T>
{   
    template <typename TStream>
    static void write(const T& val, TStream&& stream)
    {
        stream.writeRange(std::ranges::subrange(val));
    }

    template <typename TStream>
    static void read(T& val, TStream&& stream)
    {
        auto r = std::ranges::subrange(val);
        stream.readRange(r);
    }
};

///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct StreamRule<std::optional<T>>
{
    template <typename TStream>
    static void write(const std::optional<T>& val, TStream&& stream)
    {
        stream.write(val.has_value());
        if (val)
            stream.write(*val);
    }

    template <typename TStream>
    static void read(std::optional<T>& val, TStream&& stream)
    {
        bool hasValue;
        stream.read(hasValue);
        if (hasValue)
        {
            T v;
            stream.read(v);
            val = std::move(v);
        }
        else
            val = std::nullopt;
    }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
constexpr bool isVersioned = []() {
    if constexpr (requires { StreamRule<T>::Versioned; })
        return StreamRule<T>::Versioned;
    else
        return false;
}();


}