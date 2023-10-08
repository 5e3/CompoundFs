#pragma once

#include "FileSystem.h"
#include "FileSystemVisitor.h"
#include "Path.h"
#include <string_view>
#include <string>

namespace TxFs
{

using FolderContents = std::vector<std::pair<PathHolder, TreeValue>>;

//////////////////////////////////////////////////////////////////////////



size_t copy(FileSystem& sourceFs, Path sourcePath, FileSystem& destFs, Path destPath);
inline size_t copy(FileSystem& sourceFs, Path sourcePath, Path destPath)
{
    return copy(sourceFs, sourcePath, sourceFs, destPath);
}

FolderContents retrieveFolderContents(Path path, const FileSystem& fs);


///////////////////////////////////////////////////////////////////////////////

}
