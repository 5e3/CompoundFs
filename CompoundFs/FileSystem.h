#pragma once

#include "DirectoryStructure.h"
#include "FileReader.h"
#include "FileWriter.h"

namespace TxFs
{
enum class WriteHandle : uint32_t;
enum class ReadHandle : uint32_t;
class Path;

//////////////////////////////////////////////////////////////////////////

class FileSystem final
{
public:
    using Cursor = DirectoryStructure::Cursor;
    using Startup = DirectoryStructure::Startup;

public:
    FileSystem(const Startup& startup);

    static Startup initialize(const std::shared_ptr<CacheManager>& cacheManager);

    std::optional<WriteHandle> createFile(Path path);
    std::optional<WriteHandle> appendFile(Path path);
    std::optional<ReadHandle> readFile(Path path);
    std::optional<uint64_t> fileSize(Path path) const;

    size_t read(ReadHandle file, void* ptr, size_t size);
    size_t write(WriteHandle file, const void* ptr, size_t size);

    void close(WriteHandle file);
    void close(ReadHandle file);
    uint64_t fileSize(WriteHandle file) const;
    uint64_t fileSize(ReadHandle file) const;

    std::optional<Folder> makeSubFolder(Path path);
    std::optional<Folder> subFolder(Path path) const;

    bool addAttribute(Path path, const TreeValue& attribute);
    std::optional<TreeValue> getAttribute(Path path) const;

    size_t remove(Path path);

    Cursor find(Path path) const;
    Cursor begin(Path path) const;
    Cursor next(Cursor cursor) const;

    void commit();
    void rollback();

private:
    void closeAllFiles();

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

///////////////////////////////////////////////////////////////////////////////

inline FileSystem::Cursor FileSystem::next(Cursor cursor) const
{
    return m_directoryStructure.next(cursor);
}

inline FileSystem::Startup FileSystem::initialize(const std::shared_ptr<CacheManager>& cacheManager)
{
    return DirectoryStructure::initialize(cacheManager);
}

}
