#pragma once

#include "DirectoryStructure.h"
#include "FileReader.h"
#include "FileWriter.h"
#include "Path.h"

namespace TxFs
{
enum class WriteHandle : uint32_t;
enum class ReadHandle : uint32_t;

//////////////////////////////////////////////////////////////////////////

class FileSystem final
{
public:
    class Cursor;
    using Startup = DirectoryStructure::Startup;

public:
    FileSystem(const Startup& startup);

    static Startup initialize(const std::shared_ptr<CacheManager>& cacheManager);
    void init();

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

    bool rename(Path oldPath, Path newPath);
    size_t remove(Path path);

    Cursor find(Path path) const;
    Cursor begin(Path path) const;
    Cursor next(Cursor cursor) const;

    void commit();
    void rollback();

    bool reducePath(Path& p) const;
    bool createPath(Path& p);

private:
    void closeAllFiles();
    FileWriter& addOpenWriter(Path path);

private:
    struct OpenWriter
    {
        PathHolder m_path;
        FileWriter m_fileWriter;
    };

    std::shared_ptr<CacheManager> m_cacheManager;
    DirectoryStructure m_directoryStructure;
    std::unordered_map<ReadHandle, FileReader> m_openReaders;
    std::unordered_map<WriteHandle, OpenWriter> m_openWriters;
    uint32_t m_nextHandle = 1;
};

///////////////////////////////////////////////////////////////////////////////

class FileSystem::Cursor final
{
    friend class FileSystem;

public:
    constexpr Cursor() noexcept = default;
    Cursor(const DirectoryStructure::Cursor& cursor) noexcept
        : m_cursor(cursor)
    {
    }

    constexpr bool operator==(const Cursor& rhs) const noexcept { return m_cursor == rhs.m_cursor; }
    constexpr bool operator!=(const Cursor& rhs) const noexcept { return !(m_cursor == rhs.m_cursor); }

    Path key() const;
    TreeValue value() const { return m_cursor.value(); }
    constexpr explicit operator bool() const noexcept { return m_cursor.operator bool(); }

private:
    DirectoryStructure::Cursor m_cursor;
};

//////////////////////////////////////////////////////////////////////////

inline Path FileSystem::Cursor::key() const
{
    return Path(m_cursor.key().first, m_cursor.key().second);
}

inline FileSystem::Cursor FileSystem::next(Cursor cursor) const
{
    return m_directoryStructure.next(cursor.m_cursor);
}

inline FileSystem::Startup FileSystem::initialize(const std::shared_ptr<CacheManager>& cacheManager)
{
    return DirectoryStructure::initialize(cacheManager);
}

inline bool FileSystem::reducePath(Path& path) const
{
    return path.normalize(&m_directoryStructure);
}

inline bool FileSystem::createPath(Path& path)
{
    return path.create(&m_directoryStructure);
}

}
