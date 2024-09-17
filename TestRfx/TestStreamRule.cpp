
#include <gtest/gtest.h>
#include "Rfx/StreamRule.h"
#include "Rfx/Stream.h"
#include "Rfx/Blob.h"


#include <ranges>
#include <tuple>
#include <variant>
#include <optional>
#include <array>


#include <string>
#include <list>
#include <set>
#include <deque>
#include <map>
#include <unordered_map>
#include <forward_list>
#include <array>
#include <optional>

using namespace Rfx;

namespace
{

///////////////////////////////////////////////////////////////////////////////
//template <typename T>
//inline constexpr bool HasStreamRule_v = requires { &StreamRule<T>::write; };

template <typename T>
inline constexpr bool HasStreamRule_v = requires(T&& val) {
    StreamRule<T>::read(val, [](auto) {});
    StreamRule<T>::write(val, [](auto) {});
};

///////////////////////////////////////////////////////////////////////////////
struct SupportsStreaming
{
    int m_i = 42;
    auto operator<=>(const SupportsStreaming&) const = default;
};

template <typename TVisitor>
void forEachMember(SupportsStreaming& ms, TVisitor&& visitor)
{
    visitor(ms.m_i);
}

struct SupportsStreamingV2
{
    int m_i = 123;
    double m_d = 1.234;
    auto operator<=>(const SupportsStreamingV2&) const = default;
};

template <typename TVisitor>
void forEachMember(SupportsStreamingV2& ms, TVisitor&& visitor)
{
    visitor(ms.m_i);
    visitor(ms.m_d);
}

///////////////////////////////////////////////////////////////////////////////
struct StructWithoutStreamingSupport
{};


///////////////////////////////////////////////////////////////////////////////
template<typename T>
struct ComplexType
{
    double m_double = 1.23;
    T m_SomeType;
    std::string m_string = "test";

    bool operator==(const ComplexType<T>& rhs) const = default;
    template<typename U> requires(!std::is_same_v<U,T>)
    bool operator==(const ComplexType<U>& rhs) const
    {
        return m_double == rhs.m_double && m_string == rhs.m_string;
    }
};

template<typename T, typename TVisitor>
void forEachMember(ComplexType<T>& val, TVisitor&& visit)
{
    visit(val.m_double);
    visit(val.m_SomeType);
    visit(val.m_string);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
Blob streamOut(const T& outVal)
{
    StreamOut out;
    out.write(outVal);
    return out.swapBlob();
}

template <typename T>
T streamIn(const Blob& blob)
{
    StreamIn in(blob);
    T val {};
    in.read(val);
    if (!in.empty())
        std::runtime_error("StreamIn is not empty");
    return val;
}

template <typename T>
void streamedInComparesEqualTo(const T& val)
{
    auto blob = streamOut(val);
    T valIn = streamIn<T>(blob);
    ASSERT_EQ(val, valIn);
}

template <typename TV1, typename TV2>
void streamVersions(const TV2& v2)
{
    ComplexType<TV2> outVal { 3.14, v2, "some string" };
    auto blob = streamOut(outVal);

    // check if streaming it in is equal
    auto inVal = streamIn<ComplexType<TV2>>(blob);
    ASSERT_EQ(outVal, inVal);

    // check if streaming into TV1 works
    auto inVal1 = streamIn<ComplexType<TV1>>(blob);
    ASSERT_EQ(outVal, inVal1);
}

template<typename TNew, typename TOld>
void streamOldVsNew(const TOld& oldVal, const TNew& newVal)
{
    streamVersions<TOld>(newVal);
    streamVersions<TNew>(oldVal);
}


}// namespace

///////////////////////////////////////////////////////////////////////////////
/// check that there is a StreamRule<> defined for these types
static_assert(HasStreamRule_v<SupportsStreaming>);
static_assert(HasStreamRule_v<std::string>);
static_assert(HasStreamRule_v<std::vector<int>>);
static_assert(HasStreamRule_v<std::list<int>>);
static_assert(HasStreamRule_v<std::deque<int>>);
static_assert(HasStreamRule_v<std::set<int>>);
static_assert(HasStreamRule_v<std::map<std::string, int>>);
static_assert(HasStreamRule_v<std::unordered_map<std::string, int>>);
static_assert(HasStreamRule_v<std::tuple<std::string, int>>);
static_assert(HasStreamRule_v<std::pair<std::string, int>>);
static_assert(HasStreamRule_v<std::array<std::string, 5>>);
static_assert(HasStreamRule_v<double[5]>);
static_assert(HasStreamRule_v<std::variant<std::string, int>>);
static_assert(HasStreamRule_v<std::optional<std::string>>);

static_assert(!HasStreamRule_v<StructWithoutStreamingSupport>);
static_assert(!HasStreamRule_v<std::forward_list<int>>); // size() is O(n) => not supported for now

static_assert(IsVersioned_v<SupportsStreaming>);
static_assert(IsVersioned_v<std::tuple<int, int>>);
static_assert(IsVersioned_v<std::variant<int, double>>);
static_assert(IsVersioned_v<std::array<double,3>>);

static_assert(!IsVersioned_v<std::pair<int, int>>);
static_assert(!IsVersioned_v<std::string>);
static_assert(!IsVersioned_v<std::vector<std::string>>);
static_assert(!IsVersioned_v<double[3]>);

///////////////////////////////////////////////////////////////////////////////
TEST(StreamRule, versionedStruct)
{
    SupportsStreaming oldVal { 123 };
    SupportsStreamingV2 newVal { 456, 7.89 };
    streamOldVsNew(oldVal, newVal);
}

TEST(StreamRule, DefaultCreatesFirstVariantTypeOnUnkownVersion)
{
    std::variant<double, int, std::string> var = "test variant";
    auto blob = streamOut(var);
    StreamIn in(blob);
    using OldVariant = std::variant<double, int>;
    OldVariant varIn = 123.4;
    in.read(varIn);
    ASSERT_EQ(varIn, OldVariant());
}

TEST(StreamRule, BuiltinArray)
{
    int arr[] = { 1, 2, 3 };
    auto blob = streamOut(arr);
    StreamIn in(blob);
    int arrIn[3];
    in.read(arrIn);

    ASSERT_TRUE(std::equal(std::begin(arr), std::end(arr), std::begin(arrIn)));
}

TEST(StreamRule, stdOptional)
{
    StreamOut out;
    std::optional<int> opt;
    std::optional<int> opt2 = 1;
    out.write(opt);
    out.write(opt2);

    auto blob = out.swapBlob();
    StreamIn in(blob);
    std::optional<int> optIn = 2;
    std::optional<int> optIn2 = 3;
    in.read(optIn);
    in.read(optIn2);

    ASSERT_EQ(opt, optIn);
    ASSERT_EQ(opt2, optIn2);
}

TEST(StreamRule, tupleIsVersionedPairIsNot)
{
    auto tup = std::make_tuple(5, 6);
    StreamOut out1;
    out1.write(tup);

    auto pa = std::make_pair(5, 6);
    StreamOut out2;
    out2.write(pa);

    ASSERT_GT(out1.swapBlob().size(), out2.swapBlob().size());
}

TEST(StreamRule, versionsOfTuplesAreExchangable)
{
     auto oldVal = std::make_tuple(1, 2);
     auto newVal = std::make_tuple(1, 2, 3);
    streamOldVsNew(oldVal, newVal);
}

TEST(StreamRule, map)
{
    std::map<int, std::string> outVal = { { 2, "Test" }, { 5, "Nochntest" } };
    streamedInComparesEqualTo(outVal);
}

TEST(StreamRule, streamingBigArrayCompiles)
{
    std::array<int, 5000> outVal {};
    streamedInComparesEqualTo(outVal); // the std::apply() version would not compile for big arrays
}

//TEST(StreamRule, versionsOfArraysAreExchangable)
//{
//    std::array<int, 3> oldVal {1,2,3};
//    std::array<int, 4> newVal {4,5,6,7};
//    streamOldVsNew(oldVal, newVal);
//}
