

#pragma once

#include <stdint.h>
#include <limits>

namespace TxFs
{
    struct  FileLockPosition
    {
          static constexpr int64_t GateBegin = std::numeric_limits<int64_t>::max() - 3LL;
          static constexpr int64_t GateEnd = GateBegin + 1LL;
          static constexpr int64_t SharedBegin = GateEnd; 
          static constexpr int64_t SharedEnd = SharedBegin + 1LL;
          static constexpr int64_t WriteBegin = SharedEnd;
          static constexpr int64_t WriteEnd = WriteBegin + 1LL;
    };
}


