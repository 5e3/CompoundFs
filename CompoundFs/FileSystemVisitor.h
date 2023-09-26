
#pragma once

#include "FileSystem.h"
#include "Path.h"
#include "SmallBufferStack.h"
#include "TreeValue.h"
#include <vector>
#include <string>
#include <string_view>

namespace
{}

namespace TxFs
{

enum class VisitorControl { Continue, Break };

class FileSystemVisitor
{
    FileSystem& m_fs;

public:
    FileSystemVisitor(FileSystem& fs)
        : m_fs(fs)
    {
    }

    template <typename TVisitor>
    void visit(Path path, TVisitor& visitor)
    {
        FileSystem::Cursor cursor = prepareVisit(path, visitor);
        SmallBufferStack<FileSystem::Cursor, 10> stack;
        while (cursor)
        {
            if (visitor(Path(cursor.key().first, cursor.key().second), cursor.value()) == VisitorControl::Break)
                return;

            auto type = cursor.value().getType();
            cursor = m_fs.next(cursor);

            if (type == TreeValue::Type::Folder)
            {
                stack.push_back(cursor);
                cursor = m_fs.begin(Path(cursor.value().toValue<Folder>(), ""));
            }

            while (!cursor && !stack.empty())
            {
                cursor = stack.back();
                stack.pop_back();
            }
        }
    }

private:
    template <typename TVisitor>
    FileSystem::Cursor prepareVisit(Path path, TVisitor& visitor)
    {
        // There is no root entry - handle it...
        if (path == RootFolder)
        {
            auto control = visitor(path, TreeValue { Folder { path.Root } });
            return control == VisitorControl::Continue ? m_fs.begin(path) : FileSystem::Cursor();
        }

        FileSystem::Cursor cursor = m_fs.find(path);
        if (!cursor)
            return cursor;

        auto control = visitor(Path(cursor.key().first, cursor.key().second), cursor.value());
        if (control == VisitorControl::Continue && cursor.value().getType() == TreeValue::Type::Folder)
            return m_fs.begin(Path(cursor.value().toValue<Folder>(), ""));

        return FileSystem::Cursor();
    }
};

///////////////////////////////////////////////////////////////////////////////

class FolderKey
{
    Folder m_folder;
    std::string m_name;

public:
    FolderKey(std::pair<Folder, std::string_view> key)
        : m_folder(key.first)
        , m_name(key.second)
    {
    }

    operator Path() const { return Path(m_folder, m_name); }
    std::string_view name() const { return m_name; }
};

//////////////////////////////////////////////////////////////////////////

struct TreeEntry
{
    FolderKey m_key;
    TreeValue m_value;
};

//////////////////////////////////////////////////////////////////////////

struct FsCompareVisitor
{
    FileSystem& m_sourceFs;
    FileSystem& m_destFs;
    Folder m_folder;
    std::string m_name;
    enum class Result
    {
        NotFound,
        NotEqual,
        Equal                                                   
    };
    Result m_result;
    SmallBufferStack<std::pair<Folder, Folder>, 10> m_stack;
    std::vector<char> m_buffer;


    FsCompareVisitor(FileSystem& sourceFs, FileSystem& destFs, Path path)
        : m_sourceFs(sourceFs)
        , m_destFs(destFs)
        , m_folder { path.m_parent}
        , m_name{path.m_relativePath}
        , m_result(Result::Equal)
    {
        if (!m_destFs.reducePath(path))
            m_result = Result::NotFound;

    }

    Path getDestPath(Path sourcePath)
    {
        if (m_stack.empty())
            return Path(m_folder, m_name);
        else
        {
            while (m_stack.back().first != sourcePath.m_parent)
                m_stack.pop_back();
            return Path(m_stack.back().second, sourcePath.m_relativePath);
        }
    }

    VisitorControl operator()(Path path, const TreeValue& value)
    {
        if (m_result != Result::Equal)
            return VisitorControl::Break;
                        
        Path destPath = getDestPath(path);

        return dispatch(path, value, destPath);
    }

    std::optional<TreeValue> getDestValue(Path destPath)
    {
        if (destPath == RootFolder)
            return TreeValue { Folder { Path::Root } };    

        auto destCursor = m_destFs.find(destPath);
        if (!destCursor)
        {
                m_result = Result::NotFound;
                return std::nullopt;
        }
        return destCursor.value();
    }

    VisitorControl dispatch(Path sourcePath, const TreeValue& sourceValue, Path destPath)
    {
        auto destValue = getDestValue(destPath);        
        if (!destValue)
        {
            m_result = Result::NotFound;
            return VisitorControl::Break;
        }

        auto sourceType = sourceValue.getType();
        auto destType = destValue->getType();
        if (sourceType != destType)
        {
            m_result = Result::NotEqual;
            return VisitorControl::Break;
        }

        if (sourceType == TreeValue::Type::Folder)
        {
            auto sourceFolder = sourceValue.toValue<Folder>();
            auto destFolder = destValue->toValue<Folder>();
            m_stack.push_back(std::pair { sourceFolder, destFolder});
        }
        else if (sourceType == TreeValue::Type::File)
            return compareFiles(sourcePath, destPath);
        else
        {
            if (sourceValue != *destValue)
            {
                m_result = Result::NotEqual;
                return VisitorControl::Break;
            }
        }
        return VisitorControl::Continue;
    }

    VisitorControl compareFiles(Path sourcePath, Path destPath)
    {
        auto sourceHandle = *m_sourceFs.readFile(sourcePath);
        auto destHandle = *m_destFs.readFile(destPath);

        if (m_sourceFs.fileSize(sourceHandle) != m_destFs.fileSize(destHandle))
        {
            m_result = Result::NotEqual;
            return VisitorControl::Break;
        }

        return compareFiles(sourceHandle, destHandle);
    }
        
    VisitorControl compareFiles(ReadHandle sourceHandle, ReadHandle destHandle)
    {
        m_buffer.reserve(32 * 4096); // 128K
        m_buffer.resize(1);
        auto data = m_buffer.data();

        size_t fsize = m_sourceFs.fileSize(sourceHandle);
        size_t blockSize = std::min(fsize, m_buffer.capacity() / 2);
        for (size_t i = 0; i < fsize; i += blockSize)
        {
            m_sourceFs.read(sourceHandle, data, blockSize);
            auto readSize = m_destFs.read(destHandle, data+blockSize, blockSize);
            if (!std::equal(data, data + readSize, data + blockSize, data + blockSize+ readSize))
            {
                m_result = Result::NotEqual;
                return VisitorControl::Break;
            }

        }
        return VisitorControl::Continue;
    }
};

}

///////////////////////////////////////////////////////////////////////////////
