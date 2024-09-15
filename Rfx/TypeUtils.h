#pragma once

#include <type_traits>
#include <concepts>
#include <tuple>
#include <ranges>


namespace Rfx
{

///////////////////////////////////////////////////////////////////////////////
/// Test for concrete Template
template<typename T, template<typename ...> class Template>
struct IsSpecialization : std::false_type {};

template <typename T, template <typename...> class Template>
inline constexpr bool IsSpecialization_v = IsSpecialization<T, Template>::value;

template <template <typename...> class Template, typename... TArgs>
struct IsSpecialization<Template<TArgs...>, Template> : std::true_type
{};

///////////////////////////////////////////////////////////////////////////////
/// Tests is a container supports an insert() method
template <typename T>
inline constexpr bool CanInsert_v = requires(T cont) { cont.insert(typename T::value_type{}); };

///////////////////////////////////////////////////////////////////////////////
/// Tests is a container supports a resize() method
template <typename T>
inline constexpr bool CanResize_v = requires(T cont) { cont.resize(size_t{}); };

///////////////////////////////////////////////////////////////////////////////
/// Tests if there exists a free standing function forEachMember(myClass, visitor)
template <typename TMyClass>
inline constexpr bool HasForEachMember_v = requires(TMyClass myClass) { forEachMember(myClass, [](TMyClass) {}); };


///////////////////////////////////////////////////////////////////////////////
/// Covers std::array, etc ?
template <typename T>
concept FixedSizeContainer = requires
{
    { std::bool_constant < (T{}.size() == T{}.max_size()) > () } -> std::same_as<std::true_type>;
};

///////////////////////////////////////////////////////////////////////////////
/// Builtin arrays and std::array
template <typename T>
concept FixedSizeArray = std::is_bounded_array_v<T>
|| (FixedSizeContainer<T> && requires (T val) { val[0]; });

///////////////////////////////////////////////////////////////////////////////
/// Covers tuples-likes (std::tuple<>, std::pair<>, etc) without std::array<> 
template<typename T>
concept TupleLike = requires { std::tuple_size<T>::value; } && (!FixedSizeContainer<T>);

///////////////////////////////////////////////////////////////////////////////
/// 
template<typename T>
concept ContainerLike = std::ranges::sized_range<T> 
    && requires(T cont) 
    {
        typename T::value_type;
        cont.clear();
    } 
    && (CanInsert_v<T> || CanResize_v<T>);

}//namespace Rfx

