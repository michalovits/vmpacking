#ifndef VMP_CLUSTERTREEINSTANCEBUILDER_H
#define VMP_CLUSTERTREEINSTANCEBUILDER_H

#include <vmp_clustertreetopology.h>
#include <vmp_guest.h>

#include <memory>
#include <unordered_set>
#include <vector>

namespace vmp
{

class ClusterTreeBuilder
{
  public:
    explicit ClusterTreeBuilder(size_t capacity);

    size_t addInnerNode(size_t cluster, std::vector<size_t> parents, std::unordered_set<int> pages);
    size_t addLeafNode(std::vector<size_t> parents, const std::shared_ptr<const Guest> &guest,
                       std::unordered_set<int> pages);
    size_t createCluster(size_t parent);

    [[nodiscard]] ClusterTreeInstance build() &&;

    [[nodiscard]] static size_t rootCluster();

  private:
    [[nodiscard]] bool allInCluster(const std::vector<size_t> &nodes, size_t cluster) const;

    size_t capacity_;
    ClusterTreeTopology topology_;
};

}  // namespace vmp

#endif  // VMP_CLUSTERTREEINSTANCEBUILDER_H
