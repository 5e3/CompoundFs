#include "../CompoundFs/Blob.h"
#include "../CompoundFs/FileDescriptor.h"
#include <memory>
#include <tuple>
#include <utility>
#include <iostream>
#include <variant>

using namespace TxFs;
//
// struct Version
//{
//    int m_major = 0;
//    int m_minor = 0;
//    int m_patch = 0;
//};
//
// struct Folder
//{
//    uint32_t m_folderId;
//};
//
// struct Empty {};
//
// template<typename Tuple>
// struct ToVariant;
//
// template<typename... Args>
// struct ToVariant<std::tuple<Args...>>
//{
//    using Variant = std::variant<Args ...>;
//
//};
//
//
// enum class DirectoryType : uint8_t { Empty, File, Folder, Version };
// using DirectoryTypeList = std::tuple<Empty, FileDescriptor, Folder, Version>;
// static const char* const Names[] = { "Empty", "File", "Folder", "Version" };
// using DirectoryVariant = ToVariant<DirectoryTypeList>::Variant;
//
// template <class T, class Tuple>
// struct Index;
//
// template <class T, class... Types>

// struct Index<T, std::tuple<T, Types...>> {
//    static const std::size_t value = 0;
//    static const DirectoryType m_type = DirectoryType::Empty;
//};
//
// template <class T, class U, class... Types>
// struct Index<T, std::tuple<U, Types...>> {
//    static const std::size_t value = 1 + Index<T, std::tuple<Types...>>::value;
//    static const DirectoryType m_type = (DirectoryType)value;
//};
//
// template<typename T>
// auto makeTuple(const T&);
//
// auto makeTuple(const Version& v)
//{
//    return std::make_tuple(v.m_major, v.m_minor, v.m_patch);
//}
//
// auto makeTuple(const Folder& v)
//{
//    return std::make_tuple(v.m_folderId);
//}
//
// auto makeTuple(const FileDescriptor& v)
//{
//    return std::make_tuple(v.m_first, v.m_last, v.m_fileSize);
//}
//
// auto makeTuple(const Empty& v)
//{
//    return std::make_tuple();
//}
//
//
//
//
//
//
//
//
//
// template <typename T, typename Tuple, std::size_t... Inds>
// T helperMakeFromTuple(Tuple&& tuple, std::index_sequence<Inds...>)
//{
//    return T{ std::get<Inds>(std::forward<Tuple>(tuple))... };
//}
//
// template <typename T, typename Tuple>
// T makeFromTuple(Tuple&& tuple)
//{
//    return helperMakeFromTuple<T>(std::forward<Tuple>(tuple),
//        std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>());
//}
//
//
// template<typename T>
// Blob makeBlob(const T& value)
//{
//    return Blob();
//}
//
// template<typename T>
// constexpr const char* getType(const T&)
//{
//    return Names[Index<T, DirectoryTypeList>::value];
//}
//
// template <typename T>
// auto variantFromBlob(const Blob& blob)
//{
//    decltype(makeTuple(T())) t;
//    auto rt = makeFromTuple<T>(t);
//    return DirectoryVariant(rt);
//
//}
//
// using variantFromBlobFunc = DirectoryVariant(*)(const Blob&);
//
// template <typename T>
// struct VariantFromBlobFuncArray;
//
// template <typename... Args>
// struct VariantFromBlobFuncArray < std::tuple<Args...>>
//{
//    static inline variantFromBlobFunc funcs[] = { variantFromBlob<Args> ... };
//};
//
// using Func = int(*)();
//
// const Func g_funcVect[] = { [] {return 5;}, [] {return 3;} };
//
// int main()
//{
//    Version v = { 1, 2, 3 };
//    //Blob b = makeBlob(v);
//    std::cout << getType(v) << "\n" << getType(FileDescriptor());
//    auto t = makeTuple(v);
//    Version v2 = makeFromTuple<Version>(t);
//    //Version v3 = std::make_from_tuple<Version>(t);
//    decltype(makeTuple(Version())) t2 = makeTuple(v2);
//
//    //VariantFromBlobFuncArray<DirectoryTypeList>::funcs[3](Blob());
//    DirectoryVariant dv;
//    auto var = variantFromBlob<Version>(Blob());
//}

#include <deque>

struct Generic
{
    std::vector<uint8_t> m_value;
};

template <typename Tuple>
struct ToVariant;

template <typename... Args>
struct ToVariant<std::tuple<Args...>>
{
    using Variant = std::variant<Args...>;
};

using Types = std::tuple<uint8_t, uint32_t, uint64_t, std::string>;
using Buffer = std::deque<uint8_t>;
using Variant = ToVariant<Types>::Variant;

template <typename T>
Variant BufferToVariant(Buffer& buf)
{
    T val;
    std::copy(buf.begin(), buf.begin() + sizeof(T), (uint8_t*) &val);
    buf.erase(buf.begin(), buf.begin() + sizeof(T));
    return val;
}

using BufferToVariantFuncType = Variant (*)(Buffer&);

template <typename T>
struct BufferToVariantFuncArray;

template <typename... Args>
struct BufferToVariantFuncArray<std::tuple<Args...>>
{
    inline static const BufferToVariantFuncType m_funcs[] = { BufferToVariant<Args>... };
};

size_t t2iHelper(size_t in, std::true_type)
{
    return in;
}

size_t t2iHelper(size_t, std::false_type)
{
    return 0;
}

template <typename Tuple>
struct TypeToIndex;

template <typename... Args>
struct TypeToIndex<std::tuple<Args...>>
{
    template <typename T>
    static constexpr size_t getIndex()
    {
        static_assert((std::is_same<T, Args>::value + ...) == 1, "Type has to match one of the tuple<> types!");
        size_t i = 0;
        return (t2iHelper(i++, std::is_same<T, Args>()) + ...);
    }
};

struct Test
{
    // static const size_t s = TypeToIndex<Types>::getIndex<uint32_t>();
};

using TestVariant = std::tuple<double, std::pair<double, double>, std::pair<int, int>, std::string>;

int main()
{
    BufferToVariantFuncArray<Types> bvfa;
    Buffer buf(100, 2);

    auto type = buf.front();
    buf.pop_front();
    bvfa.m_funcs[type](buf);

    auto i = TypeToIndex<TestVariant>::getIndex<double>();

    //TestVariant tv = std::make_pair(1, 2);
    //tv = std::make_pair(1., 2.);
    //tv = "Test";

    auto boolSum = true + true;
}
