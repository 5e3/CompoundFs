

#pragma once

#include <stdint.h>
#include <limits>

namespace TxFs
{
    struct  FileLockPosition
    {
          static constexpr int64_t PageSize = 4096LL; 
          static constexpr int64_t MaxFileSize = int64_t(std::numeric_limits<uint32_t>::max() - 1) * PageSize;
          static constexpr int64_t GateBegin = std::numeric_limits<int64_t>::max() - 3;
          static constexpr int64_t GateEnd = GateBegin + 1LL;
          static constexpr int64_t SharedBegin = GateEnd; 
          static constexpr int64_t SharedEnd = SharedBegin + 1LL;
          static constexpr int64_t WriteBegin = SharedEnd;
          static constexpr int64_t WriteEnd = WriteBegin + 1;
    };
}


