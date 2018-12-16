
#ifndef IOMANAGERINTERFACE_H
#define IOMANAGERINTERFACE_H

#include <vector>

class IoManagerInterface
{
public:
  typedef std::vector<unsigned char> Cluster;
  enum Access { readMode, writeMode };

public:
  virtual ~IoManagerInterface() {}

  virtual void extend(size_t clusters) = 0;
  virtual void reduce(size_t clusters) = 0;

  virtual void write(size_t clusterNr, const Cluster& cluster) = 0;
  virtual void read(size_t clusterNr, Cluster& cluster) const = 0;

  virtual size_t getClusterSize() const = 0;
  virtual size_t getNumClusters() const = 0;
};



#endif //IOMANAGERINTERFACE_H

                                                                                                        