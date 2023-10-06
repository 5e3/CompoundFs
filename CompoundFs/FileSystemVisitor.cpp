#include "FileSystemVisitor.h"
#include <filesystem>

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
        while (m_stack.top().first != sourcePath.m_parent)
            m_stack.pop();
        return Path(m_stack.top().second, sourcePath.m_relativePath);
    }
}

std::optional<TreeValue> FsCompareVisitor::getDestValue(Path destPath)
{
    if (destPath == RootPath)
        return TreeValue { Folder::Root };

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
        m_stack.push(std::pair { sourceFolder, destFolder });
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

char* FsCompareVisitor::getLazyMemoryBuffer()
{
    if (!m_buffer)
        m_buffer = std::make_unique<char[]>(BufferSize);
    return m_buffer.get();
}

VisitorControl FsCompareVisitor::compareFiles(ReadHandle sourceHandle, ReadHandle destHandle)
{
    auto data = getLazyMemoryBuffer(); 

    size_t fsize = m_sourceFs.fileSize(sourceHandle);
    size_t blockSize = std::min(fsize, BufferSize / 2);
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

///////////////////////////////////////////////////////////////////////////////

struct TempFileBuffer::Impl
{
    std::filesystem::path m_path;
    FILE* m_file;
    std::unique_ptr<uint8_t[]> m_buffer;
    static constexpr size_t BufferSize = 4096;
    size_t m_bufferPos;
    size_t m_bufferEnd;

    static std::filesystem::path createTempFilePath()
    {
        return std::filesystem::temp_directory_path() / std::tmpnam(nullptr);
    }

    Impl()
        : m_path(createTempFilePath())
        , m_file(fopen(m_path.string().c_str(), "w+"))
        , m_buffer(std::make_unique < uint8_t[]>(BufferSize))
        , m_bufferPos(0)
    {
        if (!m_file)
            throw std::system_error(EDOM, std::system_category());
    }

    ~Impl() 
    { 
        fclose(m_file);
        std::error_code errorCode;
        std::filesystem::remove(m_path, errorCode);
    }

    void write(Path path, const TreeValue& value)
    {
        DirectoryKey key(path.m_parent, path.m_relativePath);
        auto pos = toStream(key, m_buffer.get());

        ByteStringStream bss;
        value.toStream(bss);
        pos = toStream(bss, pos);
        fwrite(m_buffer.get(), pos - m_buffer.get(), 1, m_file);
    }
    std::optional<TreeEntry> startReading()
    {
        fseek(m_file, 0, SEEK_SET);
        m_bufferEnd = fread(m_buffer.get(), 1, BufferSize, m_file);
        return read();
    }

    std::optional<TreeEntry> read()
    {
        if (m_bufferPos == m_bufferEnd)
            return std::nullopt;

        const auto* pos = &m_buffer[m_bufferPos];
        ByteStringView bsv = ByteStringView::fromStream(pos);
        pos = bsv.end();

        Folder folder;
        bsv = ByteStringStream::pop(folder, bsv);
        Path path(folder, std::string_view((char*) bsv.data(), bsv.size()));

        bsv = ByteStringView::fromStream(pos);
        pos = bsv.end();
        m_bufferPos = pos - m_buffer.get();
        TreeValue tv = TreeValue::fromStream(bsv);

        auto ret = TreeEntry { FolderKey { path }, tv };
        reload();

        return ret;
    }

    void reload()
    {
        if (m_bufferEnd < BufferSize)
            return;

        if (BufferSize - m_bufferPos >= 512)
            return;

        auto pos = std::copy(m_buffer.get() + m_bufferPos, m_buffer.get() + m_bufferEnd, m_buffer.get());
        pos += fread(pos, 1, m_bufferPos, m_file);
        m_bufferPos = 0;
        m_bufferEnd = pos - m_buffer.get();
    }
};

TxFs::TempFileBuffer::TempFileBuffer() = default;
TxFs::TempFileBuffer::~TempFileBuffer() = default;

void TxFs::TempFileBuffer::write(Path path, const TreeValue& value)
{
    if (!m_impl)
        m_impl = std::make_unique<Impl>();

    m_impl->write(path, value);
}

std::optional<TreeEntry> TxFs::TempFileBuffer::startReading()
{
    if (!m_impl)
        return std::nullopt;

    return m_impl->startReading();
}

std::optional<TreeEntry> TxFs::TempFileBuffer::read()
{
    if (!m_impl)
        return std::nullopt;

    return m_impl->read();
}

size_t TxFs::TempFileBuffer::getBufferSize() const
{
    return Impl::BufferSize;
}

size_t TxFs::TempFileBuffer::getFileSize() const
{
    return size_t(ftell(m_impl->m_file));
}

