
#include "../CompoundFs/InnerNode.h"
#include "../CompoundFs/PageAllocator.h"
#include <memory>

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

int main()
{
    PageAllocator pa(0);
    auto in = PageBuilder::construct<InnerNode>(pa.allocate(), "Key", 1, 2);
    auto id = in->findPage("Key");

    in = PageBuilder::construct<InnerNode>(pa.allocate());
}
