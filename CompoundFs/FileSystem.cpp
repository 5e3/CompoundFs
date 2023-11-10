
#include "FileSystem.h"
#include "Path.h"

using namespace TxFs;

struct FileSystem::RollbackOnException
{
    FileSystem& m_fileSystem;
    int m_uncaughtExceptions;

    RollbackOnException(FileSystem& fs)
        : m_fileSystem(fs)
        , m_uncaughtExceptions(std::uncaught_exceptions())
    {
    }

    ~RollbackOnException() 
    {
        if (std::uncaught_exceptions() > m_uncaughtExceptions)
        {
            try
            {
                m_fileSystem.rollback();
            }
            catch (...)
            {
            }
        }
    }
};

FileSystem::FileSystem(const Startup& startup)
    : m_cacheManager(startup.m_cacheManager)
    , m_directoryStructure(startup)
{}

std::optional<WriteHandle> FileSystem::createFile(Path path)
{
    RollbackOnException guard(*this);

    if (!path.create(&m_directoryStructure))
        return std::nullopt;

    if (!m_directoryStructure.createFile(DirectoryKey(path.m_parentFolder, path.m_relativePath)))
        return std::nullopt;

    addOpenWriter(path);
    return WriteHandle { m_nextHandle++ };
}

std::optional<WriteHandle> FileSystem::appendFile(Path path)
{
    RollbackOnException guard(*this);

    if (!path.create(&m_directoryStructure))
        return std::nullopt;

    auto fileDescriptor = m_directoryStructure.appendFile(DirectoryKey(path.m_parentFolder, path.m_relativePath));

    if (!fileDescriptor)
        return std::nullopt;

    auto& fileWriter = addOpenWriter(path);
    if (*fileDescriptor != FileDescriptor())
        fileWriter.openAppend(*fileDescriptor);
    return WriteHandle { m_nextHandle++ };
}

std::optional<ReadHandle> FileSystem::readFile(Path path)
{
    if (!path.normalize(&m_directoryStructure))
        return std::nullopt;

    auto fileDescriptor = m_directoryStructure.openFile(DirectoryKey(path.m_parentFolder, path.m_relativePath));

    if (!fileDescriptor)
        return std::nullopt;

    auto res = m_openReaders.try_emplace(ReadHandle { m_nextHandle }, FileReader { m_cacheManager });
    assert(res.second);
    if (*fileDescriptor != FileDescriptor())
        res.first->second.open(*fileDescriptor);
    return ReadHandle { m_nextHandle++ };
}

std::optional<uint64_t> FileSystem::fileSize(Path path) const
{
    if (!path.normalize(&m_directoryStructure))
        return std::nullopt;

    auto fileDescriptor = m_directoryStructure.openFile(DirectoryKey(path.m_parentFolder, path.m_relativePath));

    if (!fileDescriptor)
        return std::nullopt;

    return fileDescriptor->m_fileSize;
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
    RollbackOnException guard(*this);

    const uint8_t* begin = (const uint8_t*) ptr;
    const uint8_t* end = begin + size;
    m_openWriters.at(file).m_fileWriter.write(begin, end);
    return size;
}

void FileSystem::close(WriteHandle file)
{
    RollbackOnException guard(*this);

    auto& openFile = m_openWriters.at(file);
    auto fileDescriptor = openFile.m_fileWriter.close();
    Path path = openFile.m_path;
    m_directoryStructure.updateFile(DirectoryKey(path.m_parentFolder, path.m_relativePath), fileDescriptor);
    m_openWriters.erase(file);
}

void FileSystem::close(ReadHandle file)
{
    [[maybe_unused]] auto res = m_openReaders.at(file); // throws if non-existant
    m_openReaders.erase(file);
}

uint64_t FileSystem::fileSize(WriteHandle file) const
{
    const auto& openFile = m_openWriters.at(file);
    return openFile.m_fileWriter.size();
}

uint64_t FileSystem::fileSize(ReadHandle file) const
{
    const auto& openFile = m_openReaders.at(file);
    return openFile.size();
}

std::optional<Folder> FileSystem::makeSubFolder(Path path)
{
    RollbackOnException guard(*this);

    if (!path.create(&m_directoryStructure))
        return std::nullopt;

    return m_directoryStructure.makeSubFolder(DirectoryKey(path.m_parentFolder, path.m_relativePath));
}

std::optional<Folder> FileSystem::subFolder(Path path) const
{
    if (!path.normalize(&m_directoryStructure))
        return std::nullopt;

    return m_directoryStructure.subFolder(DirectoryKey(path.m_parentFolder, path.m_relativePath));
}

bool FileSystem::addAttribute(Path path, const TreeValue& attribute)
{
    RollbackOnException guard(*this);

    if (!path.create(&m_directoryStructure))
        return false;

    return m_directoryStructure.addAttribute(DirectoryKey(path.m_parentFolder, path.m_relativePath), attribute);
}

std::optional<TreeValue> FileSystem::getAttribute(Path path) const
{
    if (!path.normalize(&m_directoryStructure))
        return std::nullopt;

    return m_directoryStructure.getAttribute(DirectoryKey(path.m_parentFolder, path.m_relativePath));
}

bool FileSystem::rename(Path oldPath, Path newPath)
{
    RollbackOnException guard(*this);

    if (!oldPath.normalize(&m_directoryStructure))
        return false;
    DirectoryKey oldKey(oldPath.m_parentFolder, oldPath.m_relativePath);

    if (!newPath.create(&m_directoryStructure))
        return false;
    DirectoryKey newKey(newPath.m_parentFolder, newPath.m_relativePath);

    return m_directoryStructure.rename(oldKey, newKey);
}

size_t FileSystem::remove(Path path)
{
    RollbackOnException guard(*this);

    if (!path.normalize(&m_directoryStructure))
        return 0;

    return m_directoryStructure.remove(DirectoryKey(path.m_parentFolder, path.m_relativePath));
}

FileSystem::Cursor FileSystem::find(Path path) const
{
    if (!path.normalize(&m_directoryStructure))
        return Cursor();

    return m_directoryStructure.find(DirectoryKey(path.m_parentFolder, path.m_relativePath));
}

FileSystem::Cursor FileSystem::begin(Path path) const
{
    if (!path.normalize(&m_directoryStructure))
        return Cursor();

    return m_directoryStructure.begin(DirectoryKey(path.m_parentFolder, path.m_relativePath));
}

void FileSystem::commit()
{
    RollbackOnException guard(*this);

    closeAllFiles();
    m_directoryStructure.commit();
}

void FileSystem::rollback()
{
    closeAllFiles();
    m_directoryStructure.rollback();
}

void FileSystem::init()
{
    m_directoryStructure.init();
}

void FileSystem::closeAllFiles()
{
    for (auto& [key, openFile]: m_openWriters)
    {
        auto fileDescriptor = openFile.m_fileWriter.close();
        Path path = openFile.m_path;
        m_directoryStructure.updateFile(DirectoryKey(path.m_parentFolder, path.m_relativePath), fileDescriptor);
    }
    m_openWriters.clear();
    m_openReaders.clear();
}

FileWriter& TxFs::FileSystem::addOpenWriter(Path path)
{
    RollbackOnException guard(*this);

    auto res = m_openWriters.try_emplace(WriteHandle { m_nextHandle },
                                         OpenWriter { PathHolder { path }, FileWriter { m_cacheManager } });

    assert(res.second);
    auto& openWriter = res.first->second;
    return openWriter.m_fileWriter;
}


