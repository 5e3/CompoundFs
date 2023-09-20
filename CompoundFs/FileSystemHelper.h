#pragma once

#include "FileSystem.h"
#include "Path.h"
#include <string_view>
#include <string>

namespace TxFs
{
class FolderKey
{
    Folder m_folder;
    std::string m_name;

public:
    FolderKey(std::pair<Folder, std::string_view> key)
        : m_folder(key.first)
        , m_name(key.second)
    {
    }

    operator Path() const { return Path(m_folder, m_name); }
};

using FolderContents = std::vector<std::pair<FolderKey, TreeValue>>;

//////////////////////////////////////////////////////////////////////////



size_t copy(FileSystem& sourceFs, Path sourcePath, FileSystem& destFs, Path destPath);
inline size_t copy(FileSystem& sourceFs, Path sourcePath, Path destPath)
{
    return copy(sourceFs, sourcePath, sourceFs, destPath);
}

FolderContents retrieveFolderContents(Path path, const FileSystem& fs);


///////////////////////////////////////////////////////////////////////////////

}
