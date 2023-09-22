
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

    size_t copyType(const TreeEntry& entry, Folder destFolder)
    {
        Path destPath(destFolder, Path(entry.m_key).m_relativePath);
        switch (entry.m_value.getType())
        {
        case TreeValue::Type::File: {
            return copyFile(entry.m_key, destPath);
        }
        case TreeValue::Type::Folder: {
            auto sourceFolder = entry.m_value.toValue<Folder>();
            auto dfolder = m_destFs.makeSubFolder(destPath);
            return dfolder ? copyFolder(sourceFolder, *dfolder) + 1 : 0;
        }
        default:
            return m_destFs.addAttribute(destPath, entry.m_value);
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
        constexpr size_t MaxEntries = 64;
        size_t numItems = 0;
        std::vector<TreeEntry> treeEntries;
        treeEntries.reserve(MaxEntries);

        auto sourceCursor = m_sourceFs.begin(Path(sourceFolder, ""));
        while (sourceCursor)
        {
            for (int i = 0; sourceCursor && i < MaxEntries; sourceCursor = m_sourceFs.next(sourceCursor), i++)
                treeEntries.push_back({ sourceCursor.key(), sourceCursor.value() });

            assert(treeEntries.size() <= MaxEntries);
            assert(treeEntries.size() > 0);

            for (const auto& entry: treeEntries)
                numItems += copyType(entry, destFolder);

            sourceCursor = m_sourceFs.find(treeEntries.back().m_key);

            if (sourceCursor)
                sourceCursor = m_sourceFs.next(sourceCursor);
            else
                sourceCursor = m_sourceFs.begin(treeEntries.back().m_key);
            treeEntries.clear();
        }

        return numItems;
    }
};
}

namespace TxFs
{
class TreeVisitor
{
    FileSystem& m_fs;

public:
    TreeVisitor(FileSystem& fs)
        : m_fs(fs)
    {
    }

    template<typename TVisitor>
    void visit(Path path, TVisitor& visitor)
    {
        auto cursor = m_fs.find(path);
        if (!cursor)
            return;

        TreeEntry entry { cursor.key(), cursor.value() };
        visitor(entry);
        if (entry.m_value.getType() == TreeValue::Type::Folder)
        {
            std::vector<TreeEntry> treeEntries;
            cursor = m_fs.find(Path(entry.m_value.toValue<Folder>(), ""));
            for (; cursor; cursor = m_fs.next(cursor))
                treeEntries.push_back({ cursor.key(), cursor.value() });
            {

            }
            entry = TreeEntry{ cursor.key(), cursor.value() };
        }
    }
};

}

///////////////////////////////////////////////////////////////////////////////

size_t TxFs::copy(FileSystem& sourceFs, Path sourcePath, FileSystem& destFs, Path destPath)
{
    CopyProcessor cp(sourceFs, destFs);
    if (sourcePath == RootFolder)
    {
        auto destFolder = destFs.makeSubFolder(destPath);
        if (!destFolder)
            return 0;

        return cp.copyFolder(sourcePath.m_parent, destPath.m_parent);
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
