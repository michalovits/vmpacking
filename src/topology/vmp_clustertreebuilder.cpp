#include <vmp_clustertreebuilder.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>

namespace vmp
{

ClusterTreeBuilder::ClusterTreeBuilder(const size_t capacity) : capacity_(capacity) {}

bool ClusterTreeBuilder::allInCluster(const std::vector<NodeId> &nids, const ClusterId cid) const
{
    const auto belongs = [&](const NodeId nid) {
        return instance_.nodes_[nid].cluster == cid;
    };
    return std::ranges::all_of(nids, belongs);
}

ClusterTreeBuilder::NodeId ClusterTreeBuilder::addInnerNode(const ClusterId cid,
                                                            std::vector<NodeId> parentNids,
                                                            std::unordered_set<int> pages)
{
    auto &nodes = instance_.nodes_;
    [[maybe_unused]] const ClusterId parentCid = instance_.clusters_[cid].parent;

    assert(allInCluster(parentNids, parentCid));

    for ([[maybe_unused]] const NodeId nid : parentNids) {
        assert(!instance_.isLeafNode(nid));
    }

    const NodeId nid = nodes.size();
    for (const NodeId parentNid : parentNids) {
        nodes[parentNid].children.push_back(nid);
    }

    nodes.emplace_back(std::move(parentNids), std::move(pages), nullptr, cid);
    instance_.clusters_[cid].nodes.push_back(nid);

    return nid;
}

ClusterTreeBuilder::NodeId ClusterTreeBuilder::addLeafNode(std::vector<NodeId> parentNids,
                                                           Guest guest,
                                                           std::unordered_set<int> pages)
{
    assert(!parentNids.empty());

    auto &nodes = instance_.nodes_;
    const ClusterId parentCid = nodes[parentNids[0]].cluster;

    assert(allInCluster(parentNids, parentCid));

    const NodeId nid = nodes.size();
    const ClusterId cid = createCluster(parentCid);

    // Link the new node to the parents
    for (const NodeId parentNid : parentNids) {
        nodes[parentNid].children.push_back(nid);
    }

    // Bookkeeping for the leaf cache
    instance_.leaves_.push_back(nid);
    guests_.push_back(std::move(guest));
    instance_.clusters_[cid].nodes.push_back(nid);

    // The guest pointer is bound in build(); store null for now.
    nodes.emplace_back(std::move(parentNids), std::move(pages), nullptr, cid);

    return nid;
}

ClusterTreeBuilder::ClusterId ClusterTreeBuilder::createCluster(const ClusterId parentCid)
{
    auto &clusters = instance_.clusters_;
    const ClusterId cid = clusters.size();

    clusters.emplace_back(parentCid, std::vector<NodeId>());
    clusters[parentCid].children.push_back(cid);

    return cid;
}

ClusterTree ClusterTreeBuilder::build() &&
{
    auto instance = std::make_shared<const Instance>(capacity_, std::move(guests_));
    const auto instanceGuests = instance->guests();

    for (size_t i = 0; i < instance_.leaves_.size(); ++i) {
        instance_.nodes_[instance_.leaves_[i]].guest = instanceGuests[i];
    }

    instance_.instance_ = std::move(instance);
    return std::move(instance_);
}

ClusterTreeBuilder::ClusterId ClusterTreeBuilder::rootCluster()
{
    return ClusterTree::rootCluster();
}

}  // namespace vmp
