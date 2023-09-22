
#pragma once

#include "FileSystem.h"
#include "Path.h"
#include "SmallBufferStack.h"
#include "TreeValue.h"
#include <vector>
#include <string>
#include <string_view>


namespace
{
}

namespace TxFs
{

class FileSystemVisitor
{
    FileSystem& m_fs;

public:
    FileSystemVisitor(FileSystem& fs)
        : m_fs(fs)
    {
    }

    template<typename TVisitor>
    FileSystem::Cursor prepareVisit(Path path, TVisitor& visitor)
    {
        // There is no root entry - handle it...
        if (path == Path(""))
        {
            visitor(path, TreeValue { Folder { path.m_root } });
            return m_fs.begin(path);
        }

        FileSystem::Cursor cursor = m_fs.find(path);
        if (!cursor)
            return cursor;

        visitor(Path(cursor.key().first, cursor.key().second), cursor.value());
        if (cursor.value().getType() == TreeValue::Type::Folder) 
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
            visitor(Path(cursor.key().first, cursor.key().second), cursor.value());

            if (cursor.value().getType() == TreeValue::Type::Folder)
            {  
                cursor = m_fs.next(cursor);
                stack.push_back(cursor);
                cursor = m_fs.begin(Path(cursor.value().toValue<Folder>(), ""));
            }
            else
                cursor = m_fs.next(cursor);

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
};


//////////////////////////////////////////////////////////////////////////

struct TreeEntry
{
    FolderKey m_key;
    TreeValue m_value;
};

}

///////////////////////////////////////////////////////////////////////////////

