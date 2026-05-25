#include <vmp_clustertreetopology.h>

#include <cassert>

namespace vmp
{

ClusterTreeTopology::ClusterTreeTopology()
{
    clusters_ = std::vector<Cluster>(ROOT_CLUSTER + 1);
    clusters_[ROOT_CLUSTER] = Cluster(ROOT_CLUSTER, {});
}

size_t ClusterTreeTopology::rootCluster()
{
    return ROOT_CLUSTER;
}

std::vector<std::shared_ptr<const Guest>> ClusterTreeTopology::guests() const
{
    std::vector<std::shared_ptr<const Guest>> guests;
    guests.reserve(leaves_.size());

    for (const auto &leaf : leaves_) {
        guests.emplace_back(guestOfNode(leaf));
    }
    return guests;
}

size_t ClusterTreeTopology::guestCount() const
{
    return leaves_.size();
}

const std::vector<size_t> &ClusterTreeTopology::nodesOfCluster(const size_t cluster) const
{
    return clusters_[cluster].nodes;
}

const std::vector<size_t> &ClusterTreeTopology::childrenOfCluster(const size_t cluster) const
{
    return clusters_[cluster].children;
}

size_t ClusterTreeTopology::parentOfCluster(const size_t cluster) const
{
    return clusters_[cluster].parent;
}

const std::vector<size_t> &ClusterTreeTopology::parentsOfNode(const size_t node) const
{
    return nodes_[node].parents;
}

const std::vector<size_t> &ClusterTreeTopology::childrenOfNode(const size_t node) const
{
    return nodes_[node].children;
}

const std::vector<size_t> &ClusterTreeTopology::leafNodes() const
{
    return leaves_;
}

size_t ClusterTreeTopology::clusterCount() const
{
    return clusters_.size();
}

const std::unordered_set<int> &ClusterTreeTopology::pagesOfNode(const size_t node) const
{
    return nodes_[node].pages;
}

const std::shared_ptr<const Guest> &ClusterTreeTopology::guestOfNode(const size_t node) const
{
    return nodes_[node].guest;
}

bool ClusterTreeTopology::isLeafNode(const size_t node) const
{
    return nodes_[node].guest != nullptr;
}

bool ClusterTreeTopology::isLeafCluster(const size_t cluster) const
{
    return clusters_[cluster].nodes.size() == 1 && isLeafNode(clusters_[cluster].nodes[0]);
}

size_t ClusterTreeTopology::nodeCount() const
{
    return nodes_.size();
}

size_t ClusterTreeTopology::nodeCountOfCluster(const size_t cluster) const
{
    return clusters_[cluster].nodes.size();
}

}  // namespace vmp
