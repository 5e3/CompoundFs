
#pragma once

#include <gtest/gtest.h>
#include <string>
#include "CompoundFs/FileSystem.h"
#include "CompoundFs/Path.h"

namespace TxFs
{

inline std::string readFile(FileSystem& fileSystem, ReadHandle handle)
{
    size_t size = (size_t) fileSystem.fileSize(handle);
    std::string data(size, ' ');
    fileSystem.read(handle, data.data(), size);
    return data;
}

inline std::string readFile(FileSystem& fsys, std::string_view path)
{
    auto handle = *fsys.readFile(path);
    size_t size = (size_t) fsys.fileSize(handle);
    std::string in(size, ' ');
    fsys.read(handle, in.data(), size);
    return in;
}

inline std::string makeFile(FileSystem& fileSystem, std::string_view path, char fillChar = '1')
{
    auto handle = *fileSystem.appendFile(path);
    std::string out(5000, fillChar);
    fileSystem.write(handle, out.data(), out.size());
    fileSystem.close(handle);
    return out;
}


struct FileSystemUtility
{
    std::string m_fileData;

    FileSystemUtility()
        : m_fileData(5000, '1')
    {
    }

    void fillFileSystem(FileSystem& fileSystem)
    {
        fileSystem.addAttribute("test/attribute", "test");
        auto handle = *fileSystem.createFile("test/file1.txt");
        fileSystem.write(handle, m_fileData.data(), m_fileData.size());
        fileSystem.close(handle);

        handle = *fileSystem.createFile("test/file2.txt");
        fileSystem.write(handle, m_fileData.data(), m_fileData.size());
        fileSystem.close(handle);
    }

    void checkFileSystem(FileSystem& fileSystem)
    {
        auto attribute = fileSystem.getAttribute("test/attribute").value();
        ASSERT_EQ(attribute.get<std::string>(), "test");

        auto handle = fileSystem.readFile("test/file1.txt").value();
        auto data = readFile(fileSystem, handle);
        ASSERT_EQ(data, m_fileData);
        data.clear();

        auto handle2 = fileSystem.readFile("test/file2.txt").value();
        auto data2 = readFile(fileSystem, handle2);
        ASSERT_EQ(data2, m_fileData);

        fileSystem.close(handle);
        fileSystem.close(handle2);
    }
};



}
