
#include "FileSystemHelper.h"
#include "FileSystem.h"
#include "Path.h"
#include <vector>

using namespace TxFs;

namespace
{
struct CopyProcessor
{
    FileSystem& m_sourceFs;
    FileSystem& m_destFs;
    std::vector<char> m_buffer;

    CopyProcessor(FileSystem& sourceFs, FileSystem& destFs)
        : m_sourceFs(sourceFs)
        , m_destFs(destFs)
        , m_buffer(4096L * 32)
    {
    }

    size_t copyType(FileSystem::Cursor sourceCursor, Path destPath)
    {
        switch (sourceCursor.value().getType())
        {
        case TreeValue::Type::File: {
            auto [folder, name] = sourceCursor.key();
            return copyFile(Path(folder, name), destPath);
        }
        case TreeValue::Type::Folder: {
            auto sourceFolder = sourceCursor.value().toValue<Folder>();
            auto destFolder = m_destFs.makeSubFolder(destPath);
            return destFolder ? copyFolder(sourceFolder, *destFolder) + 1 : 0;
        }
        default:
            return m_destFs.addAttribute(destPath, sourceCursor.value());
        }
    }

    bool copyFile(Path sourcePath, Path destPath)
    {
        auto readHandle = m_sourceFs.readFile(sourcePath);
        if (!readHandle)
            return 0;

        auto writeHandle = m_destFs.createFile(destPath);
        if (!writeHandle)
            return 0;

        bool succ = copyPhysicalFile(*readHandle, *writeHandle);
        m_sourceFs.close(*readHandle);
        m_destFs.close(*writeHandle);

        return succ;
    }

    bool copyPhysicalFile(ReadHandle readHandle, WriteHandle writeHandle)
    {
        size_t fsize = m_sourceFs.fileSize(readHandle);
        size_t blockSize = std::min(fsize, m_buffer.size());
        for (size_t i = 0; i < fsize; i += blockSize)
        {
            m_sourceFs.read(readHandle, m_buffer.data(), blockSize);
            m_destFs.write(writeHandle, m_buffer.data(), blockSize);
        }
        return true;
    }

    size_t copyFolder(Folder sourceFolder, Folder destFolder)
    {
        size_t numItems = 0;
        auto sourceCursor = m_sourceFs.begin(Path(sourceFolder, ""));
        for (; sourceCursor; sourceCursor = m_sourceFs.next(sourceCursor))
        {
            Path destPath(destFolder, sourceCursor.key().second);
            numItems += copyType(sourceCursor, destPath);
        }

        return numItems;
    }
};
}

///////////////////////////////////////////////////////////////////////////////

size_t TxFs::copy(FileSystem& sourceFs, Path sourcePath, FileSystem& destFs, Path destPath)
{
    CopyProcessor cp(sourceFs, destFs);
    if (sourcePath == Path(""))
    {
        auto destFolder = destFs.makeSubFolder(destPath);
        if (!destFolder)
            return 0;
            
        return cp.copyFolder(sourcePath.m_root, destPath.m_root);
    }

    auto sourceCursor = sourceFs.find(sourcePath);
    if (!sourceCursor)
        return 0;

    if (!destFs.createPath(destPath))
        return 0;

    return cp.copyType(sourceCursor, destPath);
}

FolderContents TxFs::retrieveFolderContents(Path path, const FileSystem& fs)
{
    FolderContents fc;
    auto res = fs.subFolder(path);
    if (!res)
        return fc;

    auto cursor = fs.begin(Path(*res, ""));
    for (; cursor; cursor = fs.next(cursor))
    {
        FolderKey k = cursor.key();
        auto v = cursor.value();
        fc.push_back(std::pair(k, v));
    }
    return fc;
}
