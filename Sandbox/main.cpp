


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

#include "CompoundFs/Hasher.h"

int main()
{
    auto dig = TxFs::hash32("Senta&Nils", 10);
    auto dig64 = TxFs::hash64("Senta&Nils", 10);
    return 0;
}
