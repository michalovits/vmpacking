#include <vmp_clustertreeinstance.h>

#include <algorithm>
#include <cassert>

namespace vmp
{

ClusterTreeInstance::ClusterTreeInstance(const size_t capacity) : capacity_(capacity)
{
    clusters_ = std::vector<Cluster>(ROOT_CLUSTER + 1);
    clusters_[ROOT_CLUSTER] = Cluster(ROOT_CLUSTER, {});
}

bool ClusterTreeInstance::allInCluster(const std::vector<size_t> &nodes, const size_t cluster) const
{
    const auto belongs = [&](const size_t node) {
        return this->nodes_[node].cluster == cluster;
    };
    return std::ranges::all_of(nodes, belongs);
}

size_t ClusterTreeInstance::addInnerNode(const size_t cluster, std::vector<size_t> parents,
                                         std::unordered_set<int> pages)
{
    assert(allInCluster(parents, clusters_[cluster].parent));

    for ([[maybe_unused]] const size_t node : parents) {
        assert(!isLeafNode(node));
    }

    const size_t newNode = nodes_.size();
    for (const size_t parent : parents) {
        nodes_[parent].children.push_back(newNode);
    }

    nodes_.emplace_back(std::move(parents), std::move(pages), nullptr, cluster);
    clusters_[cluster].nodes.push_back(newNode);

    return newNode;
}

size_t ClusterTreeInstance::addLeafNode(std::vector<size_t> parents,
                                        const std::shared_ptr<const Guest> &guest,
                                        std::unordered_set<int> pages)
{
    assert(!parents.empty());

    const size_t parentCluster = nodes_[parents[0]].cluster;
    assert(allInCluster(parents, parentCluster));

    const size_t node = nodes_.size();
    const size_t cluster = createCluster(parentCluster);

    // Link the new node to the parents
    for (const size_t parent : parents) {
        nodes_[parent].children.push_back(node);
    }

    // Bookkeeping for the leaf cache
    this->leaves_.push_back(node);
    clusters_[cluster].nodes.push_back(node);

    nodes_.emplace_back(std::move(parents), std::move(pages), guest, cluster);

    return node;
}

size_t ClusterTreeInstance::createCluster(const size_t parent)
{
    const size_t newCluster = clusters_.size();

    clusters_.emplace_back(parent, std::vector<size_t>());
    clusters_[parent].children.push_back(newCluster);

    return newCluster;
}

size_t ClusterTreeInstance::rootCluster()
{
    return ROOT_CLUSTER;
}

std::vector<std::shared_ptr<const Guest>> ClusterTreeInstance::guests() const
{
    std::vector<std::shared_ptr<const Guest>> guests;
    guests.reserve(leaves_.size());

    for (const auto &leaf : leaves_) {
        guests.emplace_back(guestOfNode(leaf));
    }
    return guests;
}

size_t ClusterTreeInstance::capacity() const
{
    return capacity_;
}

const std::vector<size_t> &ClusterTreeInstance::nodesOfCluster(const size_t cluster) const
{
    return clusters_[cluster].nodes;
}

const std::vector<size_t> &ClusterTreeInstance::childrenOfCluster(const size_t cluster) const
{
    return clusters_[cluster].children;
}

size_t ClusterTreeInstance::parentOfCluster(const size_t cluster) const
{
    return clusters_[cluster].parent;
}

const std::vector<size_t> &ClusterTreeInstance::parentsOfNode(const size_t node) const
{
    return nodes_[node].parents;
}

const std::vector<size_t> &ClusterTreeInstance::childrenOfNode(const size_t node) const
{
    return nodes_[node].children;
}

const std::vector<size_t> &ClusterTreeInstance::leafNodes() const
{
    return this->leaves_;
}

size_t ClusterTreeInstance::clusterCount() const
{
    return this->clusters_.size();
}

const std::unordered_set<int> &ClusterTreeInstance::pagesOfNode(const size_t node) const
{
    return nodes_[node].pages;
}

const std::shared_ptr<const Guest> &ClusterTreeInstance::guestOfNode(const size_t node) const
{
    return nodes_[node].guest;
}

bool ClusterTreeInstance::isLeafNode(const size_t node) const
{
    return nodes_[node].guest != nullptr;
}

bool ClusterTreeInstance::isLeafCluster(const size_t cluster) const
{
    return clusters_[cluster].nodes.size() == 1 && isLeafNode(clusters_[cluster].nodes[0]);
}

size_t ClusterTreeInstance::nodeCount() const
{
    return nodes_.size();
}

size_t ClusterTreeInstance::nodeCountOfCluster(const size_t cluster) const
{
    return clusters_[cluster].nodes.size();
}

}  // namespace vmp