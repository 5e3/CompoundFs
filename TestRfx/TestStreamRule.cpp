
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

template <typename T>
constexpr bool hasStreamRule = requires(T val) {
    StreamRule<T>::read(val, [](T) {});
    StreamRule<T>::write(val, [](T) {});
};


///////////////////////////////////////////////////////////////////////////////

struct MyStruct
{
    int m_i = 42;
};

template <typename TVisitor>
void forEachMember(MyStruct& ms, TVisitor&& visitor)
{
    visitor(ms.m_i);
}

struct MyOtherStruct
{};


}


static_assert(hasStreamRule<MyStruct>);
static_assert(hasStreamRule<std::string>);
static_assert(hasStreamRule<std::vector<int>>);
static_assert(hasStreamRule<std::list<int>>);
static_assert(hasStreamRule<std::deque<int>>);
static_assert(hasStreamRule<std::set<int>>);
static_assert(hasStreamRule<std::map<std::string, int>>);
static_assert(hasStreamRule<std::tuple<std::string, int>>);
static_assert(hasStreamRule<std::pair<std::string, int>>);
static_assert(hasStreamRule<std::array<std::string, 5>>);
static_assert(hasStreamRule<std::variant<std::string, int>>);
static_assert(hasStreamRule<std::optional<std::string>>);

static_assert(!hasStreamRule<MyOtherStruct>);
static_assert(!hasStreamRule<std::forward_list<int>>);

static_assert(isStdTuple<std::tuple<int, int, int>>);
static_assert(!isStdTuple<std::pair<int, int>>);

static_assert(isVersioned<MyStruct>);
static_assert(isVersioned<std::tuple<int, int>>);
static_assert(isVersioned<std::variant<int, double>>);

static_assert(!isVersioned<std::pair<int, int>>);
static_assert(!isVersioned<std::string>);


template<typename T>
void streamInOut(const T& val)
{
    StreamOut out;
    out.write(val);

    auto blob = out.swapBlob();
    StreamIn in(blob);
    T valIn;
    in.read(valIn);
    ASSERT_EQ(val, valIn);
}

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
    streamInOut(outVal);
}

TEST(StreamRule, array)
{
    std::array<int, 256> outVal {};
    streamInOut(outVal);
}
