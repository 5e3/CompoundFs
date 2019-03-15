
#pragma once

#include "CacheManager.h"
#include <memory>
#include <cstdint>

namespace TxFs
{
class DirectoryStructure
{
public:
    DirectoryStructure(const std::shared_ptr<CacheManager>& cacheManager, PageIndex rootIndex = PageIdx::INVALID,
                       uint32_t maxFolderId = 0);
};
}
