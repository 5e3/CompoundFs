
#include <gtest/gtest.h>
#include "Rfx/StreamRule.h"

#include <string>
#include <list>
#include <set>
#include <deque>
#include <map>
#include <unordered_map>
#include <forward_list>
#include <array>




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

static_assert(!hasStreamRule<MyOtherStruct>);
static_assert(!hasStreamRule<std::forward_list<int>>);

static_assert(isStdTuple<std::tuple<int, int, int>>);
static_assert(!isStdTuple<std::pair<int, int>>);

static_assert(isVersioned<MyStruct>);
static_assert(isVersioned<std::tuple<int, int>>);

static_assert(!isVersioned<std::pair<int, int>>);
static_assert(!isVersioned<std::string>);
