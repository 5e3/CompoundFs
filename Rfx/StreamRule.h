
#pragma once

#include <ranges>
#include <tuple>
#include <variant>
#include <optional>
#include <array>


namespace Rfx
{


///////////////////////////////////////////////////////////////////////////////

template <typename T>
constexpr bool canInsert = requires(T cont) { cont.insert(typename T::value_type {}); };

template <typename T>
constexpr bool canResize = requires(T cont) { cont.resize(size_t {}); };

template <typename T>
constexpr bool hasForEachMember = requires(T val) { forEachMember(val, [](T) {}); };

template <typename T>
struct _isTuple : std::false_type {};
template <typename... Ts>
struct _isTuple<std::tuple<Ts...>> : std::true_type {};

template <typename T>
constexpr bool isStdTuple = _isTuple<T>::value;

///////////////////////////////////////////////////////////////////////////////


template <typename T>
concept ContainerLike = std::ranges::sized_range<T> && requires(T cont) {
    typename T::value_type;
    cont.clear();
} && (canInsert<T> || canResize<T>);

template <typename T>
concept TupleLike = requires(T val) {
    std::tuple_size<T>::value;
    std::get<0>(val);
};

///////////////////////////////////////////////////////////////////////////////
// StreamRule defines read()/write() methods to for certain types. 
// primery template is left undefined
template <typename T>
struct StreamRule;

///////////////////////////////////////////////////////////////////////////////
// restricted StreamRule for user defined types. They require a freestanding function
// called 
// template<typename TVisitor> void forEachMember(MyType& val, TVisitor&& visitor);
template <typename T>
    requires hasForEachMember<T>
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
// StreamRule for STL-like containers.
template <ContainerLike TCont>
struct StreamRule<TCont>
{
    template <typename TStream>
    static void write(const TCont& cont, TStream&& stream)
    {
        typename std::remove_reference<TStream>::type::SizeType size { cont.size() };
        stream.write(size);
        stream.writeRange(cont);
    }

    template <typename TStream>
    static void read(TCont& cont, TStream&& stream)
        requires canResize<TCont>
    {
        typename std::remove_reference<TStream>::type::SizeType size {};
        stream.read(size);
        cont.resize(size);
        stream.readRange(cont);
    }

    template <typename TStream>
    static void read(TCont& cont, TStream&& stream)
        requires canInsert<TCont>
    {
        cont.clear();
        typename std::remove_reference<TStream>::type::SizeType size {};
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
    static constexpr bool Versioned = isStdTuple<T>;

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
// StreamRule for std::variant<>
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
        typename std::remove_reference<TStream>::type::SizeType index { var.index() };
        stream.write(index);
        std::visit([&stream ](const auto& value) { stream.write(value); }, var);
    }

    template<typename TStream>
    static void read(std::variant<Ts...>& var, TStream&& stream)
    {
        typename std::remove_reference<TStream>::type::SizeType index {};
        stream.read(index);
        if (index < sizeof...(Ts))
        {
            var = defaultCreateVariantByIndex(index);
            std::visit([&stream](auto& value) { stream.read(value); }, var);
        }
        else
            var = defaultCreateVariantByIndex(0);
    }
};

///////////////////////////////////////////////////////////////////////////////
// StreamRule for builtin arrays.
template <typename T, size_t N>
struct StreamRule<T[N]>
{
    using Array = T[N];

    template <typename TStream>
    static void write(const Array& val, TStream&& stream)
    {
        stream.writeRange(std::ranges::subrange(val));
    }

    template <typename TStream>
    static void read(Array& val, TStream&& stream)
    {
        auto r = std::ranges::subrange(val);
        stream.readRange(r);
    }
};
///////////////////////////////////////////////////////////////////////////////
// StreamRule for std::optional<>.
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