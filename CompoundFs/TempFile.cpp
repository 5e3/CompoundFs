
#include "TempFile.h"

#pragma warning(disable : 4996) // disable "'tmpnam': This function or variable may be unsafe."


std::filesystem::path TxFs::Private::createTempFileName()
{
    return std::filesystem::temp_directory_path() / std::tmpnam(nullptr);
}
