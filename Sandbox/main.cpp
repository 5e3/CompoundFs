


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

int main()
{
    //std::string txt = loadFile();
    //LZ4F_cctx* context = nullptr;
    //auto error = LZ4F_createCompressionContext(&context, LZ4F_VERSION);
    //
    //std::vector<unsigned char> buffer(2*1000 * 1000);

    //auto size = buffer.size();
    //size -=  LZ4F_compressBegin(context, buffer.data(), size, nullptr);
    //auto src = txt.data();
    //while (true)
    //{
    //    auto written = LZ4F_compressUpdate(context, buffer.data(), buffer.size(), src, 16*4096, nullptr);
    //    src += 16*4096;
    //    if (written)
    //        std::cout << src - txt.data() << "," << written << "\n";
    //}

    //std::cout << std::endl;


}

