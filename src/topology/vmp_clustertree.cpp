#include <vmp_clustertree.h>

#include <cassert>

namespace vmp
{

ClusterTree::ClusterTree()
{
    clusters_ = std::vector<Cluster>(ROOT_CLUSTER + 1);
    clusters_[ROOT_CLUSTER] = Cluster(ROOT_CLUSTER, {});
}

ClusterTree::ClusterId ClusterTree::rootCluster()
{
    return ROOT_CLUSTER;
}

std::span<const Guest *const> ClusterTree::guests() const
{
    return instance_->guests();
}

size_t ClusterTree::guestCount() const
{
    return instance_->guestCount();
}

size_t ClusterTree::capacity() const
{
    return instance_->capacity();
}

const std::vector<ClusterTree::NodeId> &ClusterTree::nodesOfCluster(const ClusterId cid) const
{
    return clusters_[cid].nodes;
}

const std::vector<ClusterTree::ClusterId> &ClusterTree::childrenOfCluster(const ClusterId cid) const
{
    return clusters_[cid].children;
}

ClusterTree::ClusterId ClusterTree::parentOfCluster(const ClusterId cid) const
{
    return clusters_[cid].parent;
}

const std::vector<ClusterTree::NodeId> &ClusterTree::parentsOfNode(const NodeId nid) const
{
    return nodes_[nid].parents;
}

const std::vector<ClusterTree::NodeId> &ClusterTree::childrenOfNode(const NodeId nid) const
{
    return nodes_[nid].children;
}

const std::vector<ClusterTree::NodeId> &ClusterTree::leafNodes() const
{
    return leaves_;
}

size_t ClusterTree::clusterCount() const
{
    return clusters_.size();
}

const std::unordered_set<int> &ClusterTree::pagesOfNode(const NodeId nid) const
{
    return nodes_[nid].pages;
}

const Guest *ClusterTree::guestOfNode(const NodeId nid) const
{
    return nodes_[nid].guest;
}

bool ClusterTree::isLeafNode(const NodeId nid) const
{
    return nodes_[nid].guest != nullptr;
}

bool ClusterTree::isLeafCluster(const ClusterId cid) const
{
    return clusters_[cid].nodes.size() == 1 && isLeafNode(clusters_[cid].nodes[0]);
}

size_t ClusterTree::nodeCount() const
{
    return nodes_.size();
}

size_t ClusterTree::nodeCountOfCluster(const ClusterId cid) const
{
    return clusters_[cid].nodes.size();
}

}  // namespace vmp
