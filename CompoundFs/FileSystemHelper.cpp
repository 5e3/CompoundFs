
#include "FileSystemHelper.h"
#include "FileSystem.h"
#include "Path.h"

using namespace TxFs;


   
FolderContents TxFs::getFolderContents(Path path, const FileSystem& fs)
{
    FolderContents fc;
    auto res = fs.subFolder(path);
    if (!res)
        return fc;

    auto cursor = fs.begin(Path(*res, ""));
    for (; cursor; cursor = fs.next(cursor))
    {
        FolderKey k = cursor.key();
        auto v = cursor.value();
        fc.push_back(std::pair(k, v));
    }
    return fc;
}