
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
template <typename T>
inline constexpr bool HasStreamRule_v = requires(T val) {
    StreamRule<T>::read(val, [](T) {});
    StreamRule<T>::write(val, [](T) {});
};

///////////////////////////////////////////////////////////////////////////////
struct SupportsStreaming
{
    int m_i = 42;
};

template <typename TVisitor>
void forEachMember(SupportsStreaming& ms, TVisitor&& visitor)
{
    visitor(ms.m_i);
}

///////////////////////////////////////////////////////////////////////////////
struct StructWithoutStreamingSupport
{};

///////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void streamedInComparesEqualTo(const T& val)
{
    StreamOut out;
    out.write(val);

    auto blob = out.swapBlob();
    StreamIn in(blob);
    T valIn;
    in.read(valIn);
    ASSERT_EQ(val, valIn);
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


TEST(StreamRule, DefaultCreatesFirstVariantTypeOnUnkownVersion)
{
    StreamOut out;
    std::variant<double, int, std::string> var = "test variant";
    out.write(var);

    auto blob = out.swapBlob();
    StreamIn in(blob);
    using OldVariant = std::variant<double, int>;
    OldVariant varIn = 123.4;
    in.read(varIn);
    ASSERT_EQ(varIn, OldVariant());
}

TEST(StreamRule, BuiltinArray)
{
    StreamOut out;
    int arr[] = { 1, 2, 3 };
    out.write(arr);

    auto blob = out.swapBlob();
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

    ASSERT_NE(out1.swapBlob(), out2.swapBlob());
}

TEST(StreamRule, map)
{
    std::map<int, std::string> outVal = { { 2, "Test" }, { 5, "Nochntest" } };
    streamedInComparesEqualTo(outVal);
}

TEST(StreamRule, array)
{
    std::array<int, 10000> outVal {};
    streamedInComparesEqualTo(outVal);
}
