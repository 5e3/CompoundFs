

#include "InMemoryIoManager.h"


InMemoryIoManager::InMemoryIoManager(size_t clusterSize)
  : m_clusterSize(clusterSize)
  , m_access(Access::writeMode)
{
}

InMemoryIoManager::InMemoryIoManager(const File& file)
  : m_clusterSize(file.at(0).size())
  , m_access(Access::readMode)
  , m_file(file)
{
}

InMemoryIoManager::File InMemoryIoManager::getFile() const
{
  return m_file;
}

void InMemoryIoManager::extend(size_t clusters)
{
  if (m_access == Access::readMode)
    throw std::runtime_error("no write access");

  m_file.resize(m_file.size() + clusters, Cluster(m_clusterSize));
}

void InMemoryIoManager::reduce(size_t clusters)
{
  if (m_access == Access::readMode)
    throw std::runtime_error("no write access");

  m_file.resize(m_file.size() - clusters);
}


void InMemoryIoManager::write(size_t clusterNr, const Cluster& cluster)
{
  if (m_access == Access::readMode)
    throw std::runtime_error("no write access");

  m_file.at(clusterNr) = cluster;
  m_file[clusterNr].resize(m_clusterSize);
}

void InMemoryIoManager::read(size_t clusterNr, Cluster& cluster) const 
{
  cluster = m_file.at(clusterNr);
}

size_t InMemoryIoManager::getClusterSize() const
{
  return m_clusterSize;
}

size_t InMemoryIoManager::getNumClusters() const
{
  return m_file.size();
}

