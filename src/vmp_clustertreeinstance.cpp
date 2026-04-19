#include <vmp_clustertreeinstance.h>

#include <cassert>

namespace vmp
{

ClusterTreeInstance::ClusterTreeInstance(const size_t capacity) : capacity(capacity)
{
    clusters = std::vector<Cluster>(ROOT_CLUSTER + 1);
    clusters[ROOT_CLUSTER] = Cluster(ROOT_CLUSTER, {});
}

bool ClusterTreeInstance::checkNodesAreInCluster(const std::vector<size_t> &nodes,
                                                 const size_t cluster) const
{
    return std::ranges::all_of(
        nodes, [&](const size_t node) { return this->nodes[node].cluster == cluster; });
}

size_t ClusterTreeInstance::addInner(const size_t cluster, const std::vector<size_t> &parents,
                                     const std::unordered_set<int> &pages)
{
    assert(checkNodesAreInCluster(parents, clusters[cluster].parent));
    for (const size_t node : parents) {
        assert(!nodeIsLeaf(node));
    }

    const size_t newNode = nodes.size();
    nodes.emplace_back(parents, pages, nullptr, cluster);
    clusters[cluster].nodes.push_back(newNode);

    for (const size_t parent : parents) {
        nodes[parent].children.push_back(newNode);
    }
    return newNode;
}

size_t ClusterTreeInstance::addLeaf(const std::vector<size_t> &parents,
                                    const std::shared_ptr<const Guest> &guest,
                                    const std::unordered_set<int> &pages)
{
    assert(!parents.empty());

    const size_t parentCluster = nodes[parents[0]].cluster;
    assert(checkNodesAreInCluster(parents, parentCluster));

    const size_t newNode = nodes.size();
    const size_t newCluster = createCluster(parentCluster);

    nodes.emplace_back(parents, pages, guest, newCluster);
    clusters[newCluster].nodes.push_back(newNode);
    this->leaves.push_back(newNode);

    for (const size_t parent : parents) {
        nodes[parent].children.push_back(newNode);

        if (std::ranges::find(clusters[parentCluster].children, newCluster) ==
            clusters[parentCluster].children.end()) {
            clusters[nodes[parent].cluster].children.push_back(newCluster);
        }
    }

    return newNode;
}

size_t ClusterTreeInstance::createCluster(const size_t parent)
{
    const size_t newCluster = clusters.size();

    clusters.emplace_back(parent, std::vector<size_t>());
    clusters[parent].children.push_back(newCluster);

    return newCluster;
}

size_t ClusterTreeInstance::getRootCluster()
{
    return ROOT_CLUSTER;
}

std::vector<std::shared_ptr<const Guest>> ClusterTreeInstance::getGuests() const
{
    std::vector<std::shared_ptr<const Guest>> guests;
    guests.reserve(leaves.size());
    for (const auto &leaf : leaves) {
        guests.emplace_back(getNodeGuest(leaf));
    }
    return guests;
}

size_t ClusterTreeInstance::getCapacity() const
{
    return capacity;
}

const std::vector<size_t> &ClusterTreeInstance::getClusterNodes(const size_t cluster) const
{
    return clusters[cluster].nodes;
}

const std::vector<size_t> &ClusterTreeInstance::getClusterChildren(const size_t cluster) const
{
    return clusters[cluster].children;
}

size_t ClusterTreeInstance::getClusterParent(const size_t cluster) const
{
    return clusters[cluster].parent;
}

const std::vector<size_t> &ClusterTreeInstance::getNodeParents(const size_t node) const
{
    return nodes[node].parents;
}

const std::vector<size_t> &ClusterTreeInstance::getNodeChildren(const size_t node) const
{
    return nodes[node].children;
}

const std::vector<size_t> &ClusterTreeInstance::getLeafNodes() const
{
    return this->leaves;
}

size_t ClusterTreeInstance::getClusterCount() const
{
    return this->clusters.size();
}

const std::unordered_set<int> &ClusterTreeInstance::getNodePages(const size_t node) const
{
    return nodes[node].pages;
}

const std::shared_ptr<const Guest> &ClusterTreeInstance::getNodeGuest(const size_t node) const
{
    return nodes[node].guest;
}

bool ClusterTreeInstance::nodeIsLeaf(const size_t node) const
{
    return nodes[node].guest != nullptr;
}

bool ClusterTreeInstance::clusterIsLeaf(const size_t cluster) const
{
    return clusters[cluster].nodes.size() == 1 && nodeIsLeaf(clusters[cluster].nodes[0]);
}

size_t ClusterTreeInstance::getNodeCount() const
{
    return nodes.size();
}

size_t ClusterTreeInstance::nodeCountOf(const size_t cluster) const
{
    return clusters[cluster].nodes.size();
}

}  // namespace vmp