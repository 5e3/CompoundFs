
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
    FileSystem::Cursor prepareVisit(Path path, TVisitor& visitor)
    {
        // There is no root entry - handle it...
        if (path == RootFolder)
        {
            auto control = visitor(path, TreeValue { Folder { path.AbsoluteRoot } });
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

    VisitorControl operator()(Path path, const TreeValue& value)
    {
        if (m_result != Result::Equal)
            return VisitorControl::Break;

        std::string_view name = m_stack.empty() ? m_name : std::string(path.m_relativePath);
        while (!m_stack.empty() && m_stack.back().first != path.m_parent)
            m_stack.pop_back();

        auto sourceCursor = m_sourceFs.find(path);
        assert(sourceCursor);
        auto destCursor = m_destFs.find(Path(m_folder, name));
        if (!destCursor)
        {
            m_result = Result::NotFound;
            return VisitorControl::Break;
        }

        return dispatchOnCursorValue(sourceCursor, destCursor);
    }

    VisitorControl dispatchOnCursorValue(FileSystem::Cursor sourceCursor, FileSystem::Cursor destCursor)
    {
        if (sourceCursor.value().getType() != destCursor.value().getType())
        {
            m_result = Result::NotEqual;
            return VisitorControl::Break;
        }

        auto type = sourceCursor.value().getType();
        if (type == TreeValue::Type::Folder)
            m_stack.push_back(
                std::pair { sourceCursor.value().toValue<Folder>(), destCursor.value().toValue<Folder>() });
        else if (type == TreeValue::Type::File)
            return compareFiles(...);
        else
        {
            if (sourceCursor.value() != destCursor.value())
            {
                m_result = Result::NotEqual;
                return VisitorControl::Break;
            }
        }
        return VisitorControl::Continue;

        cursor.value().getType()
        if (value.getType() == TreeValue::Type::Folder)
        {
            auto folder = m_fs.subFolder(Path(m_folder, name));
            if (!folder)
            {
                m_result = Result::NotFound;
                return VisitorControl::Break;
            }
            m_stack.push_back(std::pair { value.toValue<Folder>(), *folder });
        }
        else

    }
};

}

///////////////////////////////////////////////////////////////////////////////
