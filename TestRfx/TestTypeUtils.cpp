

#include "Rfx/TypeUtils.h"
#include <vector>
#include <tuple>
#include <array>
#include <variant>
#include <set>
#include <list>
#include <forward_list>

using namespace Rfx;

///////////////////////////////////////////////////////////////////////////////
/// IsSpecialization_v<>
static_assert(IsSpecialization_v<std::variant<int, double>, std::variant>);
static_assert(IsSpecialization_v<std::vector<int>, std::vector>);

static_assert(!IsSpecialization_v<std::pair<int, double>, std::tuple>);

///////////////////////////////////////////////////////////////////////////////
static_assert(CanInsert_v<std::set<int>>);
static_assert(!CanInsert_v<std::vector<int>>);

///////////////////////////////////////////////////////////////////////////////
static_assert(CanResize_v<std::vector<int>>);
static_assert(!CanResize_v<std::set<int>>);

///////////////////////////////////////////////////////////////////////////////
namespace
{

struct MyStruct
{
    int m_int;
};

template <typename TVisitor>
void forEachMember(MyStruct& ms, TVisitor&& visitor)
{
    visitor(ms.m_int);
}

struct MyOtherStruct {};

}// namespace 

static_assert(HasForEachMember_v<MyStruct>);

static_assert(!HasForEachMember_v<MyOtherStruct>); // missing forEachMember()

///////////////////////////////////////////////////////////////////////////////
static_assert(FixedSizeContainer<std::array<int, 5>>);

static_assert(!FixedSizeContainer<int[3]>); // not a container
static_assert(!FixedSizeContainer<std::vector<int>>);
static_assert(!FixedSizeContainer<std::vector<int>::iterator>);
static_assert(!FixedSizeContainer<std::list<int>>);
static_assert(!FixedSizeContainer<std::tuple<int, double>>);
static_assert(!FixedSizeContainer<std::variant<int, double>>);

///////////////////////////////////////////////////////////////////////////////
static_assert(FixedSizeArray<int[3]>);
static_assert(FixedSizeArray<std::array<int, 3>>);

static_assert(!FixedSizeArray<int[]>);
static_assert(!FixedSizeArray<int*>);
static_assert(!FixedSizeArray<int>);
static_assert(!FixedSizeArray<std::vector<int>>);
static_assert(!FixedSizeArray<std::vector<int>::iterator>);
static_assert(!FixedSizeArray<std::tuple<int>>);
static_assert(!FixedSizeArray<std::variant<int, double>>);

///////////////////////////////////////////////////////////////////////////////
static_assert(TupleLike<std::tuple<int, double>>);
static_assert(TupleLike<std::pair<int, double>>);

static_assert(!TupleLike<std::array<int, 5>>);
static_assert(!TupleLike<std::vector<int>>);
static_assert(!TupleLike<std::vector<int>::iterator>);
static_assert(!TupleLike<std::variant<int, double>>);


///////////////////////////////////////////////////////////////////////////////
static_assert(DynamicContainer<std::vector<int>>);
static_assert(DynamicContainer<std::list<int>>);
static_assert(DynamicContainer<std::set<int>>);

static_assert(!DynamicContainer<std::array<int, 5>>);      // not dynamic
static_assert(!DynamicContainer<std::forward_list<int>>);  // size() is not amortized constant time
static_assert(!DynamicContainer<std::variant<int, double>>);
static_assert(!DynamicContainer<std::tuple<int>>);
static_assert(!DynamicContainer<std::vector<int>::iterator>);

///////////////////////////////////////////////////////////////////////////////
namespace
{
struct StructWithSizeType{ using SizeType = char;};
}

static_assert(std::is_same_v<SizeType_t<double>, size_t>);
static_assert(std::is_same_v<SizeType_t<StructWithSizeType>, char>);
static_assert(std::is_same_v<SizeType_t<const StructWithSizeType&>, char>);
