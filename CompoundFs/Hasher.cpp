
#include "Hasher.h"
#include <xxhash.h>


uint32_t TxFs::hash32(const void* p, size_t size)
{
    return (uint32_t) XXH3_64bits(p, size);
}

uint64_t TxFs::hash64(const void* p, size_t size)
{
    return XXH3_64bits(p, size);
}
