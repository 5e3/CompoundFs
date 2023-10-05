
#pragma once

#include "FileSystem.h"
#include "Path.h"
#include "SmallBufferStack.h"
#include "TreeValue.h"
#include <vector>
#include <string>
#include <string_view>

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

            if (type == TreeValue::Type::Folder)
            {
                stack.push(m_fs.next(cursor));
                cursor = m_fs.begin(Path(cursor.value().toValue<Folder>(), ""));
            }
            else
                cursor = m_fs.next(cursor);

            while (!cursor && !stack.empty())
            {
                cursor = stack.top();
                stack.pop();
            }
        }

        visitor.done();
    }

private:
    template <typename TVisitor>
    FileSystem::Cursor prepareVisit(Path path, TVisitor& visitor)
    {
        // There is no root entry - handle it...
        if (path == RootPath)
        {
            auto control = visitor(path, TreeValue { Folder { path.RootFolder } });
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

    explicit FolderKey(Path path)
        : m_folder(path.m_parent)
        , m_name(path.m_relativePath)
    {
    }

    operator Path() const { return Path(m_folder, m_name); }
    std::string_view name() const { return m_name; }

    bool operator==(const FolderKey& lhs) const
    {
        return std::tie(m_folder, m_name) == std::tie(lhs.m_folder, lhs.m_name);
    }
};

//////////////////////////////////////////////////////////////////////////

struct TreeEntry
{
    FolderKey m_key;
    TreeValue m_value;

    bool operator==(const TreeEntry& lhs) const
    { 
        return std::tie(m_key, m_value) == std::tie(lhs.m_key, lhs.m_value);
    }
};

//////////////////////////////////////////////////////////////////////////

class FsCompareVisitor
{
public:
    enum class Result { NotFound, NotEqual, Equal };

private:
    FileSystem& m_sourceFs;
    FileSystem& m_destFs;
    Folder m_folder;
    std::string m_name;
    Result m_result;
    SmallBufferStack<std::pair<Folder, Folder>, 10> m_stack;
    std::unique_ptr<char[]> m_buffer;
    static constexpr size_t BufferSize = 32 * 4096;

public:
    FsCompareVisitor(FileSystem& sourceFs, FileSystem& destFs, Path path)
        : m_sourceFs(sourceFs)
        , m_destFs(destFs)
        , m_folder { path.m_parent}
        , m_name{path.m_relativePath}
        , m_result(Result::Equal)
    {
    }

    VisitorControl operator()(Path path, const TreeValue& value);

    template <typename TIterator=std::nullptr_t>
    void done(TIterator begin = nullptr, TIterator end = nullptr)
    {
        if constexpr (!std::is_null_pointer_v<TIterator>)
            for (; begin != end; ++begin)
                operator()(begin->m_key, begin->m_value);
    }


    Result result() const { return m_result; }

private:
    Path getDestPath(Path sourcePath);
    std::optional<TreeValue> getDestValue(Path destPath);
    VisitorControl dispatch(Path sourcePath, const TreeValue& sourceValue, Path destPath);
    VisitorControl compareFiles(Path sourcePath, Path destPath);
    char* getLazyMemoryBuffer();    
    VisitorControl compareFiles(ReadHandle sourceHandle, ReadHandle destHandle);
};

///////////////////////////////////////////////////////////////////////////////

template <typename TSink>
class FsBufferSink
{
    std::vector<TreeEntry> m_buffer;
    TSink m_chainedSink;

public:
    template <typename... TArgs> //, typename = decltype(TSink(std::declval<Args>()...))>
    FsBufferSink(size_t size, TArgs&&... args)
        : m_chainedSink(std::forward<TArgs>(args)...)
    {
        m_buffer.reserve(size);
    }

    VisitorControl operator()(Path path, const TreeValue& value)
    {
        m_buffer.push_back({ FolderKey { path }, value });
        if (m_buffer.size() == m_buffer.capacity())
        {
            for (auto& e: m_buffer)
                m_chainedSink(e.m_key, e.m_value);
            m_buffer.clear();
        }
        return VisitorControl::Continue;
    }

    template <typename TIterator = std::nullptr_t>
    void done(TIterator begin = nullptr, TIterator end = nullptr)
    {
        if constexpr (!std::is_null_pointer_v<TIterator>)
            for (; begin != end; ++begin)
                operator()(begin->m_key, begin->m_value);

        m_chainedSink.done(m_buffer.begin(), m_buffer.end());
    }

    const TSink& getChainedSink() const { return m_chainedSink; }
};

///////////////////////////////////////////////////////////////////////////////

class TempFileBuffer
{
    struct Impl;
    std::unique_ptr<Impl> m_impl;

public:
    TempFileBuffer();
    ~TempFileBuffer();
    void write(Path path, const TreeValue& value);
    std::optional<TreeEntry> startReading();
    std::optional<TreeEntry> read();
    size_t getBufferSize() const;
    size_t getFileSize() const;

};

template <typename TSink>
class FsFileBufferSink
{
    TempFileBuffer m_tempFileBuffer;
    TSink m_chainedSink;

public:
    template <typename... TArgs> //, typename = decltype(TSink(std::declval<Args>()...))>
    FsFileBufferSink(TArgs&&... args)
        : m_chainedSink(std::forward<TArgs>(args)...)
    {
    }

    VisitorControl operator()(Path path, const TreeValue& value)
    {
        m_tempFileBuffer.write(path, value);
        return VisitorControl::Continue;
    }

    template <typename TIterator = std::nullptr_t>
    void done(TIterator begin = nullptr, TIterator end = nullptr)
    {
        auto entry = m_tempFileBuffer.startReading();
        while (entry)
        {
            m_chainedSink(entry->m_key, entry->m_value);
            entry = m_tempFileBuffer.read();
        }

        if constexpr (!std::is_null_pointer_v<TIterator>)
            for (; begin != end; ++begin)
                m_chainedSink(begin->m_key, begin->m_value);

        m_chainedSink.done();
    }

    const TSink& getChainedSink() const { return m_chainedSink; }
};

}

