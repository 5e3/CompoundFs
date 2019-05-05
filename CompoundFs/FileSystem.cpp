
#include "FileSystem.h"
#include "Path.h"

using namespace TxFs;

FileSystem::FileSystem(const std::shared_ptr<CacheManager>& cacheManager, FileDescriptor freeStore, PageIndex rootIndex,
                       uint32_t maxFolderId)
    : m_cacheManager(cacheManager)
    , m_directoryStructure(cacheManager, freeStore, rootIndex, maxFolderId)
{}

std::optional<WriteHandle> FileSystem::createFile(Path path)
{
    if (!path.create(&m_directoryStructure))
        return std::nullopt;

    if (!m_directoryStructure.createFile(DirectoryKey(path.m_root, path.m_relativePath)))
        return std::nullopt;

    auto res = m_openWriters.try_emplace(
        WriteHandle { m_nextHandle },
        OpenWriter { path.m_root, std::string(path.m_relativePath), FileWriter { m_cacheManager } });

    assert(res.second);
    return WriteHandle { m_nextHandle++ };
}

std::optional<WriteHandle> FileSystem::appendFile(Path path)
{
    if (!path.create(&m_directoryStructure))
        return std::nullopt;

    auto fileDescriptor = m_directoryStructure.appendFile(DirectoryKey(path.m_root, path.m_relativePath));

    if (!fileDescriptor)
        return std::nullopt;

    auto res = m_openWriters.try_emplace(
        WriteHandle { m_nextHandle },
        OpenWriter { path.m_root, std::string(path.m_relativePath), FileWriter { m_cacheManager } });

    assert(res.second);
    if (*fileDescriptor != FileDescriptor())
        res.first->second.m_fileWriter.openAppend(*fileDescriptor);
    return WriteHandle { m_nextHandle++ };
}

std::optional<ReadHandle> FileSystem::readFile(Path path)
{
    if (!path.reduce(&m_directoryStructure))
        return std::nullopt;

    auto fileDescriptor = m_directoryStructure.openFile(DirectoryKey(path.m_root, path.m_relativePath));

    if (!fileDescriptor)
        return std::nullopt;

    auto res = m_openReaders.try_emplace(ReadHandle { m_nextHandle }, FileReader { m_cacheManager });
    assert(res.second);
    if (*fileDescriptor != FileDescriptor())
        res.first->second.open(*fileDescriptor);
    return ReadHandle { m_nextHandle++ };
}

size_t FileSystem::read(ReadHandle file, void* ptr, size_t size)
{
    uint8_t* begin = (uint8_t*) ptr;
    uint8_t* end = begin + size;
    auto cur = m_openReaders.at(file).read(begin, end);
    return cur - begin;
}

size_t FileSystem::write(WriteHandle file, const void* ptr, size_t size)
{
    const uint8_t* begin = (const uint8_t*) ptr;
    const uint8_t* end = begin + size;
    m_openWriters.at(file).m_fileWriter.write(begin, end);
    return size;
}

void FileSystem::close(WriteHandle file)
{
    auto& openFile = m_openWriters.at(file);
    auto fileDescriptor = openFile.m_fileWriter.close();
    m_directoryStructure.updateFile(DirectoryKey(openFile.m_folder, openFile.m_name), fileDescriptor);
    m_openWriters.erase(file);
}

void FileSystem::close(ReadHandle file)
{
    [[maybe_unused]] auto res = m_openReaders.at(file); // throws if non-existant
    m_openReaders.erase(file);
}

std::optional<Folder> FileSystem::makeSubFolder(Path path)
{
    if (!path.create(&m_directoryStructure))
        return std::nullopt;

    return m_directoryStructure.makeSubFolder(DirectoryKey(path.m_root, path.m_relativePath));
}

std::optional<Folder> FileSystem::subFolder(Path path) const
{
    if (!path.reduce(&m_directoryStructure))
        return std::nullopt;

    return m_directoryStructure.subFolder(DirectoryKey(path.m_root, path.m_relativePath));
}

bool FileSystem::addAttribute(Path path, const ByteStringOps::Variant& attribute)
{
    if (!path.create(&m_directoryStructure))
        return false;

    return m_directoryStructure.addAttribute(DirectoryKey(path.m_root, path.m_relativePath), attribute);
}

std::optional<ByteStringOps::Variant> FileSystem::getAttribute(Path path) const
{
    if (!path.reduce(&m_directoryStructure))
        return std::nullopt;

    return m_directoryStructure.getAttribute(DirectoryKey(path.m_root, path.m_relativePath));
}

size_t FileSystem::remove(Path path)
{
    if (!path.reduce(&m_directoryStructure))
        return 0;

    return m_directoryStructure.remove(DirectoryKey(path.m_root, path.m_relativePath));
}

FileSystem::Cursor FileSystem::find(Path path) const
{
    if (!path.reduce(&m_directoryStructure))
        return Cursor();

    return m_directoryStructure.find(DirectoryKey(path.m_root, path.m_relativePath));
}

FileSystem::Cursor FileSystem::begin(Path path) const
{
    if (!path.reduce(&m_directoryStructure))
        return Cursor();

    return m_directoryStructure.begin(DirectoryKey(path.m_root, path.m_relativePath));
}

void FileSystem::commit()
{
    closeAllFiles();
}

void FileSystem::closeAllFiles()
{
    for (auto& [key, openFile]: m_openWriters)
    {
        auto fileDescriptor = openFile.m_fileWriter.close();
        m_directoryStructure.updateFile(DirectoryKey(openFile.m_folder, openFile.m_name), fileDescriptor);
    }
    m_openWriters.clear();
    m_openReaders.clear();
}
