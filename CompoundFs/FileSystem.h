#pragma once

#include "DirectoryStructure.h"

namespace TxFs
{
enum class WriteFile : uint32_t;
enum class ReadFile : uint32_t;

//////////////////////////////////////////////////////////////////////////

struct Path
{
    static constexpr Folder AbsoluteRoot = DirectoryKey::Root;

    Path(std::string_view absolutePath)
        : m_relativePath(absolutePath)
        , m_root(AbsoluteRoot)
    {}

    Path(Folder root, std::string_view relativePath)
        : m_relativePath(relativePath)
        , m_root(root)
    {}

    std::string_view m_relativePath;
    Folder m_root;
};

//////////////////////////////////////////////////////////////////////////

class PathWalker
{
public:
    std::optional<ByteStringView> expandPath(Path path, const DirectoryStructure* ds);
    std::optional<ByteStringView> createPath(Path path, DirectoryStructure* ds);

private:
    DirectoryKey m_dkey;
};

//////////////////////////////////////////////////////////////////////////

class FileSystem
{
public:
    WriteFile createFile(Path path);
    WriteFile appendFile(Path path);
    ReadFile readFile(Path path) const;

    size_t read(ReadFile file, void* ptr, size_t size) const;
    size_t write(WriteFile file, const void* ptr, size_t size);

    void close(WriteFile file);
    void close(ReadFile file);

private:
    std::optional<Path> expandPath(Path path) const;
    std::optional<Path> createPath(Path path);

private:
    DirectoryStructure m_directoryStructure;
    // std::vector<
};

}
