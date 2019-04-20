
#include "FileSystem.h"

using namespace TxFs;

bool Path::create(DirectoryStructure* ds)
{
    auto pos = m_relativePath.find('/');
    auto relativePath = m_relativePath;
    auto root = m_root;

    while (pos != std::string_view::npos)
    {
        std::string_view dir = relativePath.substr(0, pos);
        DirectoryKey dkey(root, dir);
        auto ret = ds->makeSubFolder(dkey);
        if (!ret)
            return false;
        root = *ret;
        relativePath.remove_prefix(pos + 1);
        pos = relativePath.find('/');
    }

    m_relativePath = relativePath;
    m_root = root;
    return true;
}

bool Path::reduce(const DirectoryStructure* ds)
{
    auto pos = m_relativePath.find('/');
    auto relativePath = m_relativePath;
    auto root = m_root;

    while (pos != std::string_view::npos)
    {
        std::string_view dir = relativePath.substr(0, pos);
        DirectoryKey dkey(root, dir);
        auto ret = ds->subFolder(dkey);
        if (!ret)
            return false;
        root = *ret;
        relativePath.remove_prefix(pos + 1);
        pos = relativePath.find('/');
    }

    m_relativePath = relativePath;
    m_root = root;
    return true;
}
