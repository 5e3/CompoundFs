

#include <ranges>
#include <tuple>


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
// called forEachMember(MyType& val, TVisitor&& visitor);
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
template <TupleLike T>
struct StreamRule<T>
{
    static constexpr bool Versioned = isStdTuple<T>;

    template<typename T>
    static T& ccast(const T& val) { return const_cast<T&>(val);} 

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

template <typename T>
constexpr bool isVersioned = []() {
    if constexpr (requires { StreamRule<T>::Versioned; })
        return StreamRule<T>::Versioned;
    else
        return false;
}();







}