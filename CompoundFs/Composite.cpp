
#include "Composite.h"
#include "RollbackHandler.h"

using namespace TxFs;




FileSystem Composite::initializeNew(std::unique_ptr<FileInterface> file)
{
    auto cacheManager = std::make_shared<CacheManager>(std::move(file));
    auto startup = FileSystem::initialize(cacheManager);
    auto fileSystem = FileSystem(startup);
    fileSystem.commit();
    assert(startup.m_freeStoreIndex == 1 && startup.m_rootIndex == 0);
    return fileSystem;
}

FileSystem Composite::initializeExisting(std::unique_ptr<FileInterface> fileInterface)
{
    auto cacheManager = std::make_shared<CacheManager>(std::move(fileInterface));
    auto rollbackHandler = cacheManager->getRollbackHandler();
    rollbackHandler.revertPartialCommit();

    FileSystem::Startup startup { cacheManager, 1, 0 };
    auto fileSystem = FileSystem(startup);
    fileSystem.rollback();
    return fileSystem;
}

FileSystem Composite::initializeReadOnly(std::unique_ptr<FileInterface> fileInterface)
{
    auto cacheManager = std::make_shared<CacheManager>(std::move(fileInterface));
    auto rollbackHandler = cacheManager->getRollbackHandler();
    rollbackHandler.virtualRevertPartialCommit();

    FileSystem::Startup startup { cacheManager, 1, 0 };
    auto fileSystem = FileSystem(startup);
    return fileSystem;
}

