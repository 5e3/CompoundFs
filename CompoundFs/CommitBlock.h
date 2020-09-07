
#include "FileDescriptor.h"
#include <string>

namespace TxFs
{
    struct CommitBlock
    {
        FileDescriptor m_freeStoreDescriptor;
        uint64_t m_compositSize = 0;
        uint32_t m_maxFolderId = 2;
        
        std::string toString() const;
        static CommitBlock fromString(std::string_view);
    };
}

