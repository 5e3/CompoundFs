
#include "../CompoundFs/InnerNode.h"
#include "../CompoundFs/PageAllocator.h"
#include <memory>
#include <utility>

class PageBuilder
{
public:
    template <typename T, class... Args>
    static std::shared_ptr<T> construct(std::shared_ptr<uint8_t> mem, Args&&... args)
    {
        auto obj = new (mem.get()) T(std::forward<Args>(args)...);
        return std::shared_ptr<T>(mem, obj);
    }
};

using namespace TxFs;

using constNewInnerPage = std::pair<std::shared_ptr<const InnerNode>, const PageIndex>;

int main()
{
    PageAllocator pa(0);
    auto in = PageBuilder::construct<InnerNode>(pa.allocate(), "Key", 1, 2);
    auto id = in->findPage("Key");

    in = PageBuilder::construct<InnerNode>(pa.allocate());
    constNewInnerPage ip = constNewInnerPage(in, PageIndex(5));
    constNewInnerPage ip2 = ip;
    ip = ip2;
    Blob b("test");
    ip.first->findPage(b);
    ip.second = 4;
}
