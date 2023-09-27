#include "FileSystemVisitor.h"

using namespace TxFs;

VisitorControl FsCompareVisitor::operator()(Path path, const TreeValue& value)
{
    Path destPath = getDestPath(path);

    return dispatch(path, value, destPath);
}

Path FsCompareVisitor::getDestPath(Path sourcePath)
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

std::optional<TreeValue> FsCompareVisitor::getDestValue(Path destPath)
{
    if (destPath == RootFolder)
        return TreeValue { Path::Root };

    auto destCursor = m_destFs.find(destPath);
    if (!destCursor)
    {
        m_result = Result::NotFound;
        return std::nullopt;
    }
    return destCursor.value();
}

VisitorControl FsCompareVisitor::dispatch(Path sourcePath, const TreeValue& sourceValue, Path destPath)
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
        m_stack.push_back(std::pair { sourceFolder, destFolder });
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

VisitorControl FsCompareVisitor::compareFiles(Path sourcePath, Path destPath)
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

char* FsCompareVisitor::getMemoryBuffer(size_t size)
{
    m_buffer.reserve(size);
    m_buffer.resize(1);
    return m_buffer.data();
}

VisitorControl FsCompareVisitor::compareFiles(ReadHandle sourceHandle, ReadHandle destHandle)
{
    auto data = getMemoryBuffer(32 * 4096); // 128K

    size_t fsize = m_sourceFs.fileSize(sourceHandle);
    size_t blockSize = std::min(fsize, m_buffer.capacity() / 2);
    for (size_t i = 0; i < fsize; i += blockSize)
    {
        m_sourceFs.read(sourceHandle, data, blockSize);
        auto readSize = m_destFs.read(destHandle, data + blockSize, blockSize);
        if (!std::equal(data, data + readSize, data + blockSize, data + blockSize + readSize))
        {
            m_result = Result::NotEqual;
            return VisitorControl::Break;
        }
    }
    return VisitorControl::Continue;
}
