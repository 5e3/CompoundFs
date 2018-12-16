
#ifndef INMEMEORYIOMANAGER_H
#define INMEMEORYIOMANAGER_H

#include "IoManagerInterface.h"

class InMemoryIoManager : public IoManagerInterface
{
public:
  typedef std::vector<Cluster> File;

public:
  InMemoryIoManager(size_t clusterSize);
  InMemoryIoManager(const File& file);

  File getFile() const;

  virtual void extend(size_t clusters) override;
  virtual void reduce(size_t clusters) override;

  virtual void write(size_t clusterNr, const Cluster& cluster) override;
  virtual void read(size_t clusterNr, Cluster& cluster) const override;

  virtual size_t getClusterSize() const override;
  virtual size_t getNumClusters() const override;

private:
  size_t m_clusterSize;
  Access m_access;
  std::vector<Cluster> m_file;

};



#endif //INMEMEORYIOMANAGER_H
