
#include "FileSystem.h"

using namespace TxFs;

void Path::create(DirectoryStructure* ds)
{
    auto pos = m_relativePath.find('/');
    while (pos != std::string_view::npos)
    {
        if (pos <= 1)
            throw std::runtime_error("Illegal folder name");

        std::string_view dir = m_relativePath.substr(0, pos);
        DirectoryKey dkey(m_root, dir);
        auto ret = ds->makeSubFolder(dkey);
        if (!ret)
            throw std::runtime_error("Cannot create folder");
        m_root = *ret;
        m_relativePath.remove_prefix(pos + 1);
        pos = m_relativePath.find('/');
    }
}

void Path::reduce(const DirectoryStructure* ds)
{
    auto pos = m_relativePath.find('/');
    while (pos != std::string_view::npos)
    {
        if (pos <= 1)
            throw std::runtime_error("Illegal folder name");

        std::string_view dir = m_relativePath.substr(0, pos);
        DirectoryKey dkey(m_root, dir);
        auto ret = ds->subFolder(dkey);
        if (!ret)
            throw std::runtime_error("Cannot find folder");
        m_root = *ret;
        m_relativePath.remove_prefix(pos + 1);
        pos = m_relativePath.find('/');
    }
}
