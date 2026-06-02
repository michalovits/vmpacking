#ifndef VMP_CLUSTERTREEBUILDER_H
#define VMP_CLUSTERTREEBUILDER_H

#include <vmp_clustertree.h>
#include <vmp_guest.h>

#include <unordered_set>
#include <vector>

namespace vmp
{

class ClusterTreeBuilder
{
  public:
    using NodeId = ClusterTree::NodeId;
    using ClusterId = ClusterTree::ClusterId;

    explicit ClusterTreeBuilder(size_t capacity);

    NodeId addInnerNode(ClusterId cid, std::vector<NodeId> parentNids,
                        std::unordered_set<int> pages);
    NodeId addLeafNode(std::vector<NodeId> parentNids, Guest guest, std::unordered_set<int> pages);
    ClusterId createCluster(ClusterId parentCid);

    [[nodiscard]] static ClusterId rootCluster();

    [[nodiscard]] ClusterTree build() &&;

  private:
    [[nodiscard]] bool allInCluster(const std::vector<NodeId> &nids, ClusterId cid) const;

    size_t capacity_;
    ClusterTree tree_;

    // Parallel to tree.leaves_
    std::vector<Guest> guests_;
};

}  // namespace vmp

#endif  // VMP_CLUSTERTREEBUILDER_H
