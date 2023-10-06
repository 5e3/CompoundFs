#pragma once

#include "DirectoryStructure.h"

namespace TxFs
{

class Path final
{
public:
    constexpr Path(std::string_view absolutePath) noexcept
        : m_relativePath(absolutePath)
        , m_parent(Folder::Root)
    {}

    constexpr Path(const char* absolutePath) noexcept
        : m_relativePath(absolutePath)
        , m_parent(Folder::Root)
    {}

    constexpr Path(Folder root, std::string_view relativePath) noexcept
        : m_relativePath(relativePath)
        , m_parent(root)
    {}

    bool create(DirectoryStructure* ds);
    bool normalize(const DirectoryStructure* ds);

    constexpr bool operator==(Path rhs) const noexcept
    {
        return std::tie(m_parent, m_relativePath) == std::tie(rhs.m_parent, rhs.m_relativePath);
    }

    constexpr bool operator!=(Path rhs) const noexcept { return !(*this == rhs); }

public:
    std::string_view m_relativePath;
    Folder m_parent;

private:
    template <typename TFunc>
    bool subFolderWalk(TFunc&& func);
};

constexpr Path RootPath { "" };

}
