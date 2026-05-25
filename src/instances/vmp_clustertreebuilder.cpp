#include <vmp_clustertreebuilder.h>

#include <algorithm>
#include <cassert>
#include <utility>

namespace vmp
{

ClusterTreeBuilder::ClusterTreeBuilder(const size_t capacity) : capacity_(capacity) {}

bool ClusterTreeBuilder::allInCluster(const std::vector<size_t> &nodes, const size_t cluster) const
{
    const auto belongs = [&](const size_t node) {
        return topology_.nodes_[node].cluster == cluster;
    };
    return std::ranges::all_of(nodes, belongs);
}

size_t ClusterTreeBuilder::addInnerNode(const size_t cluster, std::vector<size_t> parents,
                                        std::unordered_set<int> pages)
{
    auto &nodes = topology_.nodes_;
    const size_t parentCluster = topology_.clusters_[cluster].parent;

    assert(allInCluster(parents, parentCluster));

    for ([[maybe_unused]] const size_t node : parents) {
        assert(!topology_.isLeafNode(node));
    }

    const size_t node = nodes.size();
    for (const size_t parent : parents) {
        nodes[parent].children.push_back(node);
    }

    nodes.emplace_back(std::move(parents), std::move(pages), nullptr, cluster);
    topology_.clusters_[cluster].nodes.push_back(node);

    return node;
}

size_t ClusterTreeBuilder::addLeafNode(std::vector<size_t> parents,
                                       const std::shared_ptr<const Guest> &guest,
                                       std::unordered_set<int> pages)
{
    assert(!parents.empty());

    auto &nodes = topology_.nodes_;
    const size_t parentCluster = nodes[parents[0]].cluster;

    assert(allInCluster(parents, parentCluster));

    const size_t node = nodes.size();
    const size_t cluster = createCluster(parentCluster);

    // Link the new node to the parents
    for (const size_t parent : parents) {
        nodes[parent].children.push_back(node);
    }

    // Bookkeeping for the leaf cache
    topology_.leaves_.push_back(node);
    topology_.clusters_[cluster].nodes.push_back(node);

    nodes.emplace_back(std::move(parents), std::move(pages), guest, cluster);

    return node;
}

size_t ClusterTreeBuilder::createCluster(const size_t parent)
{
    auto &clusters = topology_.clusters_;
    const size_t cluster = clusters.size();

    clusters.emplace_back(parent, std::vector<size_t>());
    clusters[parent].children.push_back(cluster);

    return cluster;
}

ClusterTreeInstance ClusterTreeBuilder::build() &&
{
    return ClusterTreeInstance(capacity_, std::move(topology_));
}

size_t ClusterTreeBuilder::rootCluster()
{
    return ClusterTreeTopology::rootCluster();
}

}  // namespace vmp
