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

///////////////////////////////////////////////////////////////////////////////

class PathHolder
{
    std::string m_value;
    Path m_path;

public:
    PathHolder()
        : m_path("") {}

    PathHolder(Folder folder, std::string relativePath)
        : m_value(std::move(relativePath))
        , m_path(folder, m_value)
    {
    }

    PathHolder(const PathHolder& other)
        : m_value(other.m_value)
        , m_path(other.m_path.m_parent, m_value)
    {
    }

    PathHolder(PathHolder&& other) noexcept
        : m_value(std::move(other.m_value))
        , m_path(other.m_path.m_parent, m_value)
    {
        other.m_path = "";
    }

    explicit PathHolder(Path path)
        : m_value(path.m_relativePath)
        , m_path(path.m_parent, m_value)
    {
    }

    PathHolder& operator=(const PathHolder& other)
    {
        if (&other == this)
            return *this;

        m_value = other.m_value;
        m_path = Path(other.m_path.m_parent, m_value);
        return *this;
    }

    PathHolder& operator=(Path path)
    {
        auto other = PathHolder(path);
        *this = std::move(other);
        return *this;
    }

    PathHolder& operator=(PathHolder&& other) noexcept
    {
        if (&other == this)
            return *this;
        m_value = std::move(other.m_value);
        m_path = Path(other.m_path.m_parent, m_value);
        other.m_path = "";
        return *this;
    }

    operator Path() const noexcept { return m_path; }
    Path getPath() const noexcept { return m_path; }
    bool operator==(const PathHolder& rhs) const { return getPath() == rhs.getPath(); }
};

}
