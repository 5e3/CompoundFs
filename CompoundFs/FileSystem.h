#pragma once

#include "DirectoryStructure.h"
#include "FileReader.h"
#include "FileWriter.h"

namespace TxFs
{
enum class WriteHandle : uint32_t;
enum class ReadHandle : uint32_t;

//////////////////////////////////////////////////////////////////////////

struct Path
{
    static constexpr Folder AbsoluteRoot = DirectoryKey::Root;

    Path(std::string_view absolutePath)
        : m_relativePath(absolutePath)
        , m_root(AbsoluteRoot)
    {}

    Path(const char* absolutePath)
        : m_relativePath(absolutePath)
        , m_root(AbsoluteRoot)
    {}

    Path(Folder root, std::string_view relativePath)
        : m_relativePath(relativePath)
        , m_root(root)
    {}

    std::string_view m_relativePath;
    Folder m_root;

    bool create(DirectoryStructure* ds);
    bool reduce(const DirectoryStructure* ds);
    bool operator==(Path rhs) const
    {
        return std::tie(m_root, m_relativePath) == std::tie(rhs.m_root, rhs.m_relativePath);
    }
    bool operator!=(Path rhs) const { return !(*this == rhs); }

private:
    template <typename TFunc>
    bool subFolderWalk(TFunc&& func);
};

//////////////////////////////////////////////////////////////////////////

class FileSystem
{
public:
    FileSystem(const std::shared_ptr<CacheManager>& cacheManager, FileDescriptor freeStore,
               PageIndex rootIndex = PageIdx::INVALID, uint32_t maxFolderId = 1);

    std::optional<WriteHandle> createFile(Path path);
    std::optional<WriteHandle> appendFile(Path path);
    std::optional<ReadHandle> readFile(Path path);

    size_t read(ReadHandle file, void* ptr, size_t size);
    size_t write(WriteHandle file, const void* ptr, size_t size);

    void close(WriteHandle file);
    void close(ReadHandle file);

private:
    struct OpenWriter
    {
        Folder m_folder;
        std::string m_name;
        FileWriter m_fileWriter;
    };

    std::shared_ptr<CacheManager> m_cacheManager;
    DirectoryStructure m_directoryStructure;
    std::unordered_map<ReadHandle, FileReader> m_openReaders;
    std::unordered_map<WriteHandle, OpenWriter> m_openWriters;
    uint32_t m_nextHandle = 1;
};

}
