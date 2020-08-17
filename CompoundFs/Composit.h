

#pragma once

#include "FileSystem.h"
#include "MemoryFile.h"

namespace TxFs
{

class Composit
{
public:
    using Result = std::variant<std::unique_ptr<FileSystem>, std::error_code>;

public:
    template<typename TFile=MemoryFile, typename... TArgs>
    Result create(TArgs&&... args);


};

}