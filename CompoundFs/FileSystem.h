#pragma once

#include "DirectoryStructure.h"

namespace TxFs
{

class FileSystem
{
public:
    int createFile(const DirectoryKey& dkey);
    int appendFile(const DirectoryKey& dkey);
    int readFile(const DirectoryKey& dkey);

private:
    //std::vector<
};

}
