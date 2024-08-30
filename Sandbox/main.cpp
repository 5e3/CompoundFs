


//template<typename>
//struct Wrapper;
//
//template<typename TRet, typename... TArgs>
//struct Wrapper <TRet(*)(TArgs...)>
//{
//
//    using TSig = TRet(*)(TArgs...);
//
//    Wrapper(TSig func)
//        : m_func(func)
//    {
//    }
//
//    template<typename TRet, typename... TArgs>
//    TRet operator()(TArgs... args)
//    {
//        return m_func(args...);
//    }
//
//    TSig m_func;
//
//};
//
//template <typename TRet, typename... TArgs>
//auto makeWrapper(TRet(func)(TArgs...))
//{
//    return [func](TArgs args...) -> TRet { return func(args...); };
//}
//
//int add(int a, int b)
//{
//    return a + b;
//}
//
//
//int main()
//{
//    auto f = makeWrapper(add);
//
//    f(1, 2);
//}

//#include "CompoundFs/Hasher.h"
//
//int main()
//{
//    auto dig = TxFs::hash32("Senta&Nils", 10);
//    auto dig64 = TxFs::hash64("Senta&Nils", 10);
//    return 0;
//}

#include <string>
#include <fstream>
#include <iostream>
#include <sstream> 
#include <vector>


//#include <lib/lz4frame.h>
//
//class Compressor
//{
//    LZ4F_cctx* m_context = nullptr;
//
//public: 
//    Compressor() { ::LZ4F_createCompressionContext(&m_context, LZ4F_VERSION); }
//    ~Compressor() { ::LZ4F_freeCompressionContext(m_context); }
//
//
//    
//
//
//};
//
//std::string loadFile()
//{
//    std::ifstream inFile("./Debug/MobyDick.txt", std::ios_base::in | std::ios_base::binary);
//    //inFile.open("inFileName"); // open the input file
//
//    std::stringstream strStream;
//    strStream << inFile.rdbuf(); // read the file
//    return strStream.str();
//
//}
//

//int main()
//{
//    //std::string txt = loadFile();
//    //LZ4F_cctx* context = nullptr;
//    //auto error = LZ4F_createCompressionContext(&context, LZ4F_VERSION);
//    //
//    //std::vector<unsigned char> buffer(2*1000 * 1000);
//
//    //auto size = buffer.size();
//    //size -=  LZ4F_compressBegin(context, buffer.data(), size, nullptr);
//    //auto src = txt.data();
//    //while (true)
//    //{
//    //    auto written = LZ4F_compressUpdate(context, buffer.data(), buffer.size(), src, 16*4096, nullptr);
//    //    src += 16*4096;
//    //    if (written)
//    //        std::cout << src - txt.data() << "," << written << "\n";
//    //}
//
//    //std::cout << std::endl;
//
//
//}
//

//#include "CompoundFs/Composite.h"
////#include "CompoundFs/WindowsFile.h"
//#include "CompoundFs/Path.h"
//
//using namespace TxFs;
//
//int main()
//{
//    //auto fs = Composite::open<WindowsFile>("c:/CompoundFs/test.compound", OpenMode::Open);
//    //auto fh = fs.appendFile(Path("autoexec.bat"));
//    //auto data = ByteStringView("test");
//    //fs.write(*fh, data.data(), data.size());
//
//}

//void example0()
//{
//    auto file = txfs::openTxFile("test.tx");
//    auto fh = file.openWriter("test.txt");
//    file.write(*fh, "data data");
//    file.setAttribute("someValue", 1.1);
//    file.commit();
//
//    //file.write(*fh, "more data"); throws exception
//}
//
//void example1()
//{
//    auto file = txfs::openTxFile("test.tx");
//    txfs::transact(file, [](txfs::Transaction tx) {
//        auto writer = tx.openWriter("test.txt");
//        *writer.write("data data");
//        tx.setAttribute("someValue", 1.1);
//    });
//
//    txfs::commit(file);
//}
//
//void example2()
//{
//    // transaction on more than one file
//    auto file1 = txfs::openTxFile("test1.tx");
//    auto file2 = txfs::openTxFile("test2.tx");
//
//    txfs::transact(file1, file2, [](txfs::Transaction tx1, txfs::Transaction tx2) {
//        auto writer1 = tx1.openWriter("temp.txt");
//        auto reader2 = tx2.openReader("test.txt");
//        auto buf = reader2->read();
//        writer1->write(buf);
//
//        auto writer2 = tx2.openWriter("temp.txt");
//        auto reader1 = tx1.openReader("test.txt");
//        auto buf2 = reader1->read();
//        writer2->write(buf2);
//
//        tx1.remove("test.txt");
//        tx1.rename("temp.txt, test.txt");
//        tx2.remove("test.txt");
//        tx2.rename("temp.txt, test.txt");
//    });
//
//    txfs::commit(file1, file2);
//}

//#include <variant>
//#include <array>
//
//struct Struct
//{
//    int m_i = 55;
//    float m_f = 3.14;
//};
//
//template <typename T, typename TVisitor>
//void visitAllMembers(T& composit, TVisitor&& visitor)
//    requires std::same_as<std::remove_const_t<T>, Struct>
//{
//    visitor(composit.m_i);
//    visitor(composit.m_f);
//}
//
//struct NochnStruct
//{
//    float m_f = 3.14;
//    std::string m_s = "hello";
//};
//
//template <typename T, typename TVisitor>
//void visitAllMembers(T& composit, TVisitor&& visitor)
//    requires std::same_as<std::remove_const_t<T>, NochnStruct>
//{
//    visitor(composit.m_f);
//    visitor(composit.m_s);
//}
//
//template <typename... Ts>
//std::variant<Ts...> defaultCreateVariant(size_t index)
//{
//    static constexpr std::array<std::variant<Ts...>(*)(), sizeof...(Ts)> defaultCreator
//        = { []() -> std::variant<Ts...> { return Ts {}; }...};
//    return defaultCreator.at(index)();
//}
//
//
//int main()
//{
//    std::variant<int, std::string> va = "hello";
//    auto va2 = defaultCreateVariant<int, std::string>(1);
//    auto va3 = defaultCreateVariant<float, std::string, int>(0);
//    auto va4 = defaultCreateVariant<float, std::string, int>(2);
//    // auto va5 = defaultCreateVariant<float, std::string, int>(3);
//
//    Struct s;
//    const NochnStruct cs;
//    visitAllMembers(s, [](auto val) { std::cout << val << "\n"; });
//    visitAllMembers(cs, [](auto val) { std::cout << val << "\n"; });
//}

//#include <ranges>
//#include <vector>
//
//namespace Rfx
//{
//template <typename T>
//constexpr bool canInsert = requires(T cont) { cont.insert(typename T::value_type {}); };
//
//template <typename T>
//constexpr bool canResize = requires(T cont) { cont.resize(size_t {}); };
//
//template <typename T>
//constexpr bool hasForEachMember = requires(T val) { forEachMember(val, [](T) {}); };
//
//template <typename T>
//concept Container = std::ranges::sized_range<T> && requires(T cont) {
//    typename T::value_type;
//    cont.clear();
//} && (canInsert<T> || canResize<T>);
//
//
//
//template <typename T>
//struct StreamRule;
//
//template <typename T>
//    requires hasForEachMember<T>
//struct StreamRule<T>
//{
//    static constexpr bool Versioned = true;
//
//    template <typename TVisitor>
//    static void write(const T& value, TVisitor&& visitor)
//    {
//        forEachMember((T&) value, std::forward<TVisitor>(visitor));
//    }
//
//    template <typename TVisitor>
//    static void read(T& value, TVisitor&& visitor)
//    {
//        forEachMember(value, std::forward<TVisitor>(visitor));
//    }
//};
//
//template <Container TCont>
//struct StreamRule<TCont>
//{
//    template <typename TStream>
//    static void write(const TCont& cont, TStream&& stream)
//    {
//        auto size = cont.size();
//        stream.write(size);
//        stream.writeRange(cont);
//    }
//
//    template <typename TStream>
//    static void read(TCont& cont, TStream&& stream)
//        requires canResize<TCont>
//    {
//        size_t size {};
//        stream.read(size);
//        cont.resize(size);
//        stream.readRange(cont);
//    }
//
//    template <typename TStream>
//    static void read(TCont& cont, TStream&& stream)
//        requires canInsert<TCont>
//    {
//        cont.clear();
//        size_t size {};
//        stream.read(size);
//        for (size_t i = 0; i < size; ++i)
//        {
//            typename TCont::value_type value {};
//            stream.read(value);
//            cont.insert(std::move(value));
//        }
//    }
//};
//
//#include <tuple>
//template<typename T>
//concept TupleLike =
//    requires(T val)
//{
//    std::tuple_size<T>::value;
//    std::get<0>(val);
//};
//
//template <TupleLike T>
//struct StreamRule<T>
//{
//    template <typename TStream>
//    static void write(const T& val, TStream&& stream)
//    {
//        std::apply([&stream](const auto&... telem) { ((stream.write(telem)), ...); }, val);
//    }
//
//    template <typename TStream>
//    static void read(T& val, TStream&& stream)
//    {
//        std::apply([&stream](auto&... telem) { ((stream.read(telem)), ...); }, val);
//    }
//};
//
//
//struct DummyStream
//{
//    template <typename T> void write(const T& value){}
//    template <typename T> void writeRange(const T& value){}
//    template <typename T> void read(T& value) {}
//    template <typename T> void readRange(T& value){}
//    template<typename T> void operator()(T value) {}
//};
//
//template <typename T>
//constexpr bool hasStreamRule = requires(T val) 
//{ 
//    StreamRule<T>::read(val, [](T) {});
//    StreamRule<T>::write(val, [](T) {});
//};
//
//
//
//}
//
//struct MyStruct
//{
//    int m_i = 42;
//};
//
//template<typename TVisitor>
//void forEachMember(MyStruct& ms, TVisitor&& visitor)
//{
//    visitor(ms.m_i);
//}
//
//struct MyOtherStruct {};
//
//
//using namespace Rfx;
//
//
//#include <string>
//#include <list>
//#include <set>
//#include <deque>
//#include <map>
//#include <unordered_map>
//#include <forward_list>
//#include <array>
//
//
//static_assert(hasStreamRule<MyStruct>);
//static_assert(hasStreamRule<std::string>);
//static_assert(hasStreamRule<std::vector<int>>);
//static_assert(hasStreamRule<std::list<int>>);
//static_assert(hasStreamRule<std::deque<int>>);
//static_assert(hasStreamRule<std::set<int>>);
//static_assert(hasStreamRule<std::map<std::string, int>>);
//static_assert(hasStreamRule<std::tuple<std::string, int>>);
//static_assert(hasStreamRule<std::pair<std::string, int>>);
//static_assert(hasStreamRule<std::array<std::string, 5>>);
//
//
//static_assert(!hasStreamRule<MyOtherStruct>);
//static_assert(!hasStreamRule<std::forward_list<int>>);
//
//int main()
//{
//    DummyStream stream;
//    using Cont = std::set<std::set<int>>;
//    Cont cont;
//    //auto cont = std::ranges::views::reverse(std::vector<int>());
//    //using Cont = decltype(cont);
//    
//    StreamRule<Cont>::write(cont, stream);
//    StreamRule<Cont>::read(cont, stream);
//}

//#include <variant>
//#include <string>
//#include <assert.h>
//
//struct A
//{
//    std::string s;
//    bool operator==(const A&) const = default;
//};
//
//struct B : A
//{
//    int i;
//    bool operator==(const B&) const = default;
//};
//
//struct V
//{
//    std::variant<A, B> v;
//    bool operator==(const V& other) const
//    {
//        const auto& otherv = other.v;
//        return std::visit([&otherv](const auto& val1) -> bool 
//            { 
//                return std::visit([&val1](const auto& val2) -> bool 
//                    { 
//                        return val1 == val2; 
//                    }, otherv);
//            }, v);
//    }
//
//};
//
//int main()
//{
//    V a { A("test1") };
//    V b { B { "test1", 5 } };
//    V c { B { "test1", 6 } };
//    V d { A { "test2" } };
//    assert(b == a);
//    assert(a == c);
//    assert(b != c);
//    assert(d != a);
//}

//#include <array>
//#include <iostream>
//
//// Define 128-bit unsigned number
//typedef std::array<uint64_t, 2> uint128_t;
//
//// Function to multiply two 64-bit numbers
//uint128_t multiply_uint64(uint64_t a, uint64_t b)
//{
//    uint64_t a_lo = (uint64_t) (a & 0xFFFFFFFF);
//    uint64_t a_hi = a >> 32;
//    uint64_t b_lo = (uint64_t) (b & 0xFFFFFFFF);
//    uint64_t b_hi = b >> 32;
//
//    uint64_t lo_lo = a_lo * b_lo;
//    uint64_t hi_lo = a_hi * b_lo;
//    uint64_t lo_hi = a_lo * b_hi;
//    uint64_t hi_hi = a_hi * b_hi;
//
//    uint64_t cross_terms = (hi_lo & 0xFFFFFFFF) + (lo_lo >> 32) + lo_hi;
//    uint64_t upper_cross_terms = (hi_lo >> 32) + (cross_terms >> 32) + hi_hi;
//
//    return { upper_cross_terms, (cross_terms << 32) | (lo_lo & 0xFFFFFFFF) };
//}
//
//// Function to multiply two 128-bit numbers
//uint128_t multiply_uint128(const uint128_t& a, const uint128_t& b)
//{
//    uint128_t lo_product = multiply_uint64(a[1], b[1]);
//    uint128_t hi_product = multiply_uint64(a[0], b[0]);
//    uint128_t cross_product1 = multiply_uint64(a[0], b[1]);
//    uint128_t cross_product2 = multiply_uint64(a[1], b[0]);
//
//    uint128_t result;
//    result[1] = lo_product[1];
//    result[0] = hi_product[0] + lo_product[0] + cross_product1[1] + cross_product2[1];
//    return result;
//}
//
//int main()
//{
//    uint128_t a = { 0ULL, 0xFFFFFFFFFFFFFFFFULL };
//    uint128_t b = { 0ULL, 2ULL };
//    uint128_t result = multiply_uint128(a, b);
//
//    std::cout << "Result: " << result[0] << " " << result[1] << std::endl;
//    return 0;
//}
//
//

#include <cstdint>
#include <ranges>
#include <functional>
#include <string>
#include <string_view>
#include <array>
#include <bit>

template <typename... Ts>
constexpr uint64_t fnv(Ts&&... vals)
{
    uint64_t hash = 14695981039346656037ULL;
    auto f = [&hash](auto&& v) {
        for (auto ch: v)
        {
            hash ^= ch;
            hash = hash * 1099511628211ULL;
        }
    };

    (f(vals), ...);
    return hash;
}


constexpr uint64_t encryptTea(uint64_t val, std::array<uint64_t,2> key)
{
    uint32_t v0 = val & 0xffffffff, v1 = val >> 32, sum = 0;
    uint32_t delta = 0x9e3779b9;
    uint32_t k0 = key[0] & 0xffffffff, k1 = key[0] >> 32, k2 = key[1] & 0xffffffff, k3 = key[1]>>32;
    for (int i = 0; i < 32; i++)
    {
        sum += delta;
        v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    }
    uint64_t res = v0 + (uint64_t(v1) << 32ULL);
    return res;
}

template<size_t size>
constexpr void encryptTeaCfb(std::array<char, size> plainText, std::array<uint64_t, 2> key)
{
    uint64_t next = encryptTea(1, key);
    std::array<uint64_t, 2> ekey = { encryptTea(2, key), encryptTea(3, key) };
    next = encryptTea(next, ekey);



}

#ifdef WIN32


#include <format>

template <typename TIter>
TIter copy(uint64_t val, TIter it)
{
    for (int i = 0; i < 8; i++)
    {
        val = std::rotl(val, 8);
        *it++ = val & 0xff;
    }
    return it;
}

template <typename TIter>
TIter copy(TIter it, uint64_t& val)
{
    val = 0;
    for (int i = 0; i < 8; i++)
        val = (val << 8) | *it++;
    return it;
}

template <typename TIn, typename TOut>
void finalXor(uint64_t val, TIn in, TOut first, TOut last)
{
    for (; first != last; ++first)
    {
        val = std::rotl(val, 8);
        *first = *in++ ^ (val & 0xff);
    }
}

template <size_t N, auto Key>
struct EncryptedString
{
    std::array<char, N + 8> m_cipherText {};
    constexpr EncryptedString(const char (&plain)[N])
    {
        uint64_t next = encryptTea(1, Key);
        auto cit = copy(next, std::begin(m_cipherText));
        std::array<uint64_t, 2> ekey = { encryptTea(2, Key), encryptTea(3, Key) };
        uint64_t val;
        auto pit = std::begin(plain);

        for (int i = 0; i < N / 8; i++)
        {
            next = encryptTea(next, ekey);
            pit = copy(pit, val);
            next ^= val;
            cit = copy(next, cit);    
        }

        if (cit != std::end(m_cipherText))
        {
            next = encryptTea(next, ekey);
            finalXor(next, pit, cit, std::end(m_cipherText));
        }
    }



    

    friend std::ostream& operator<<(std::ostream& out, const EncryptedString& es) 
    { 
        out << '{';
        for (int i = 0; i < es.m_cipherText.size(); i++)
            out << std::format(" {:x} ", (unsigned char) es.m_cipherText[i]);
        out << '}';
        return out;
    }
};

#pragma optimize("", off)
template<typename T>
__forceinline constexpr T xor1(T a, T b)
{
    return ((a | b) & ~(a & b));
}
#pragma optimize("", on)

static_assert((3 ^ 5) == xor1(3, 5));

int main()
{
    EncryptedString < 14, std::array<uint64_t,2>{ fnv(__TIME__), fnv(__TIME__ "est") }> es("0123456789012");
    std::cout << es << "\n";

    std::cout << fnv("t") << "\n";
    std::cout << fnv("te") << "\n";
    std::cout << fnv("tes") << "\n";
    std::cout << fnv("test") << "\n";


    std::cout << fnv(__TIME__ __FILE__) << "\n";

    constexpr auto key = fnv(__TIME__);
    constexpr auto res = encryptTea(0ULL, { fnv(__TIME__), fnv(__TIME__ "test") });
    std::cout << std::format("{:x}", res);
    //    std::cout << encryptTea(0, fnv_128_hash(__TIME__)) << "\n";
}

#else
int main()
{

}

#endif