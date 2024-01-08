


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

#include <variant>
#include <array>

struct Struct
{
    int m_i = 55;
    float m_f = 3.14;
};

template <typename T, typename TVisitor>
void visitAllMembers(T& composit, TVisitor&& visitor)
    requires std::same_as<std::remove_const_t<T>, Struct>
{
    visitor(composit.m_i);
    visitor(composit.m_f);
}

struct NochnStruct
{
    float m_f = 3.14;
    std::string m_s = "hello";
};

template <typename T, typename TVisitor>
void visitAllMembers(T& composit, TVisitor&& visitor)
    requires std::same_as<std::remove_const_t<T>, NochnStruct>
{
    visitor(composit.m_f);
    visitor(composit.m_s);
}

template <typename... Ts>
std::variant<Ts...> defaultCreateVariant(size_t index)
{
    static constexpr std::array<std::variant<Ts...>(*)(), sizeof...(Ts)> defaultCreator
        = { []() -> std::variant<Ts...> { return Ts {}; }...};
    return defaultCreator.at(index)();
}


int main()
{
    std::variant<int, std::string> va = "hello";
    auto va2 = defaultCreateVariant<int, std::string>(1);
    auto va3 = defaultCreateVariant<float, std::string, int>(0);
    auto va4 = defaultCreateVariant<float, std::string, int>(2);
    // auto va5 = defaultCreateVariant<float, std::string, int>(3);

    Struct s;
    const NochnStruct cs;
    visitAllMembers(s, [](auto val) { std::cout << val << "\n"; });
    visitAllMembers(cs, [](auto val) { std::cout << val << "\n"; });
}
