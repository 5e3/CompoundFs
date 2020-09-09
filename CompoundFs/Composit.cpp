
#include "Composit.h"

using namespace TxFs;




FileSystem Composit::initializeNew(std::unique_ptr<FileInterface> file)
{
    auto cacheManager = std::make_shared<CacheManager>(std::move(file));
    auto startup = FileSystem::initialize(cacheManager);
    auto fileSystem = FileSystem(startup);
    fileSystem.commit();
    assert(startup.m_freeStoreIndex == 1 && startup.m_rootIndex == 0);
    return fileSystem;
}

FileSystem Composit::initializeExisting(std::unique_ptr<FileInterface> fileInterface)
{
    auto cacheManager = std::make_shared<CacheManager>(std::move(fileInterface));
    auto logs = cacheManager->readLogs();
    auto* file = cacheManager->getFileInterface();
    for (auto [orig, cpy]: logs)
        TxFs::copyPage(file, cpy, orig);
    file->flushFile();

    FileSystem::Startup startup { cacheManager, 1, 0 };
    auto fileSystem = FileSystem(startup);
    fileSystem.rollback();
    return fileSystem;
}

FileSystem Composit::initializeReadOnly(std::unique_ptr<FileInterface> file)
{
    throw std::runtime_error("not implemented");
}

