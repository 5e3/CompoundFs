#pragma once

#include "DirectoryStructure.h"

namespace TxFs
{

class Path
{
public:
    static constexpr Folder AbsoluteRoot = DirectoryKey::Root;

    constexpr Path(std::string_view absolutePath) noexcept
        : m_relativePath(absolutePath)
        , m_root(AbsoluteRoot)
    {}

    constexpr Path(const char* absolutePath) noexcept
        : m_relativePath(absolutePath)
        , m_root(AbsoluteRoot)
    {}

    constexpr Path(Folder root, std::string_view relativePath) noexcept
        : m_relativePath(relativePath)
        , m_root(root)
    {}

    bool create(DirectoryStructure* ds);
    bool reduce(const DirectoryStructure* ds);

    constexpr bool operator==(Path rhs) const noexcept
    {
        return std::tie(m_root, m_relativePath) == std::tie(rhs.m_root, rhs.m_relativePath);
    }

    constexpr bool operator!=(Path rhs) const noexcept { return !(*this == rhs); }

public:
    std::string_view m_relativePath;
    Folder m_root;

private:
    template <typename TFunc>
    bool subFolderWalk(TFunc&& func);
};

}
