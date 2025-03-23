

#pragma once

#include <variant>
#include <string>
#include "../Rfx/StreamRule.h"

namespace VersionedStructs
{

///////////////////////////////////////////////////////////////////////////////

template<typename... Ts>
struct PlaceHolder
{
    std::variant<Ts...> m_variant;

    bool operator==(const PlaceHolder& other) const
    {
        const auto& otherVariant = other.m_variant;
        return std::visit(
            [&otherVariant](const auto& value1) -> bool {
                return std::visit([&value1](const auto& value2) -> bool 
                    { 
                        return value1 == value2; 
                    }, otherVariant);
            }, m_variant);
    }
};

///////////////////////////////////////////////////////////////////////////////



template <int Index>
struct A;

template <>
struct A<0>
{
    int i = -100;
    bool operator==(const A&) const = default;

    template<typename TVisitor>
    friend void forEachMember(A& val, TVisitor&& visitor)
    {
        visitor(val.i);
    }
};


template <>
struct A<1> : A<0>
{
    size_t j = 10101010;
    bool operator==(const A&) const = default;
    
    template <typename TVisitor>
    friend void forEachMember(A& val, TVisitor&& visitor)
    {
        forEachMember((A<0>&) val, visitor);
        visitor(val.j);
    }
};

template <>
struct A<2> : A<1>
{
    std::string k = "kkk";
    bool operator==(const A&) const = default;

    template <typename TVisitor>
    friend void forEachMember(A& val, TVisitor&& visitor)
    {
        forEachMember((A<1>&) val, visitor);
        visitor(val.k);
    }
};


///////////////////////////////////////////////////////////////////////////////

template <int Index>
struct B;

template <>
struct B<0>
{
    int i;
    PlaceHolder<A<0>,A<1>,A<2>> m_a;
    bool operator==(const B&) const = default;

    template <int Index>
    B(const A<Index>& a)
        : m_a(a)
    {
    }

    template <typename TVisitor>
    friend void forEachMember(B& val, TVisitor&& visitor)
    {
        visitor(val.i);
        visitor(val.m_a);
    }
};

template <>
struct B<1> : B<0>
{
    std::string j = "jjj";
    bool operator==(const B&) const = default;

    template <int Index>
    B(const A<Index>& a)
        : B<0>(a)
    {
    }

    template <typename TVisitor>
    friend void forEachMember(B& val, TVisitor&& visitor)
    {
        forEachMember((B<0>&) val, visitor);
        visitor(val.j);
    }
};

template <>
struct B<2> : B<1>
{
    size_t k = -12;
    bool operator==(const B&) const = default;

    template <int Index>
    B(const A<Index>& a)
        : B<1>(a)
    {
    }
    template <typename TVisitor>
    friend void forEachMember(B& val, TVisitor&& visitor)
    {
        forEachMember((B<1>&) val, visitor);
        visitor(val.k);
    }
};

///////////////////////////////////////////////////////////////////////////////

template <int Index>
struct C;

template <>
struct C<0>
{
    std::string i = "xxx";
    PlaceHolder<B<0>,B<1>,B<2>> m_b;
    bool operator==(const C&) const = default;

    template <int Index>
    C(const B<Index>& b)
        : m_b(b)
    {
    }

    template <typename TVisitor>
    friend void forEachMember(C& val, TVisitor&& visitor)
    {
        visitor(val.i);
        visitor(val.m_b);
    }
};

template <>
struct C<1> : C<0>
{
    int j = -1;
    bool operator==(const C&) const = default;

    template <int Index>
    C(const B<Index>& b)
        : C<0>(b)
    {
    }

    template <typename TVisitor>
    friend void forEachMember(C& val, TVisitor&& visitor)
    {
        forEachMember((C<0>&) val, visitor);
        visitor(val.j);
    }
};

template <>
struct C<2> : C<1>
{
    std::vector<std::string> k = { "yyy", "zzz" };
    bool operator==(const C&) const = default;

    template <int Index>
    C(const B<Index>& b)
        : C<1>(b)
    {
    }

    template <typename TVisitor>
    friend void forEachMember(C& val, TVisitor&& visitor)
    {
        forEachMember((C<1>&) val, visitor);
        visitor(val.k);
    }
};

///////////////////////////////////////////////////////////////////////////////

}

namespace Rfx
{
template <typename... Ts>
struct StreamRule<VersionedStructs::PlaceHolder<Ts...>>
{
    static VersionedStructs::PlaceHolder<Ts...> defaultCreateByIndex(size_t index)
    {
        return VersionedStructs::PlaceHolder<Ts...> { StreamRule<std::variant<Ts...>>::defaultCreateByIndex(index) };
    }
    template <typename TStream>
    static void write(const VersionedStructs::PlaceHolder<Ts...>& var, TStream&& stream)
    {
        std::visit([&stream](const auto& value) { stream.write(value); }, var.m_variant);
    }
    template <typename TStream>
    static void read(VersionedStructs::PlaceHolder<Ts...>& var, TStream&& stream)
    {
        std::visit([&stream](auto& value) { stream.read(value); }, var.m_variant);
    }
};

}


