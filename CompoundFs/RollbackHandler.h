
#include "Cache.h"
#include <vector>

namespace TxFs
{

class RollbackHandler final
{
public:
    RollbackHandler(Cache& cache) noexcept;

    void revertPartialCommit();
    void rollback(size_t compositeSize);
    void virtualRevertPartialCommit();
    std::vector<std::pair<PageIndex, PageIndex>> readLogs() const;

private:
    Cache& m_cache;
};

}
