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

FolderContents getFolderContents(Path path, const FileSystem& fs);

//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

}
