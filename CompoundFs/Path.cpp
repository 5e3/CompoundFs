
#include "Path.h"

using namespace TxFs;

bool Path::create(DirectoryStructure* ds)
{
    return subFolderWalk([ds = ds](const auto& dkey) { return ds->makeSubFolder(dkey); });
}

bool Path::normalize(const DirectoryStructure* ds)
{
    return subFolderWalk([ds = ds](const auto& dkey) { return ds->subFolder(dkey); });
}

template <typename TFunc>
bool Path::subFolderWalk(TFunc&& func)
{
    auto pos = m_relativePath.find('/');
    auto relativePath = m_relativePath;
    auto root = m_parentFolder;

    while (pos != std::string_view::npos)
    {
        std::string_view dir = relativePath.substr(0, pos);
        DirectoryKey dkey(root, dir);
        auto ret = func(dkey);
        if (!ret)
            return false;
        root = *ret;
        relativePath.remove_prefix(pos + 1);
        pos = relativePath.find('/');
    }

    m_relativePath = relativePath;
    m_parentFolder = root;
    return true;
}
