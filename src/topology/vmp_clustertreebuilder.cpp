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
    const auto inCluster = [&](const NodeId nid) {
        return tree_.nodes_[nid].cluster == cid;
    };
    return std::ranges::all_of(nids, inCluster);
}

ClusterTreeBuilder::NodeId ClusterTreeBuilder::addInnerNode(const ClusterId cid,
                                                            std::vector<NodeId> parentNids,
                                                            std::unordered_set<int> pages)
{
    auto &nodes = tree_.nodes_;
    [[maybe_unused]] const ClusterId parentCid = tree_.clusters_[cid].parent;

    assert(allInCluster(parentNids, parentCid));

    const NodeId nid = nodes.size();
    for (const NodeId parentNid : parentNids) {
        nodes[parentNid].children.push_back(nid);
    }

    nodes.emplace_back(std::move(parentNids), std::move(pages), nullptr, cid);
    tree_.clusters_[cid].nodes.push_back(nid);

    return nid;
}

ClusterTreeBuilder::NodeId ClusterTreeBuilder::addLeafNode(std::vector<NodeId> parentNids,
                                                           Guest guest,
                                                           std::unordered_set<int> pages)
{
    assert(!parentNids.empty());

    auto &nodes = tree_.nodes_;
    const ClusterId parentCid = nodes[parentNids[0]].cluster;

    assert(allInCluster(parentNids, parentCid));

    const NodeId nid = nodes.size();
    const ClusterId cid = createCluster(parentCid);

    // Link the new node to the parents
    for (const NodeId parentNid : parentNids) {
        nodes[parentNid].children.push_back(nid);
    }

    // Bookkeeping for the leaf cache
    tree_.leaves_.push_back(nid);
    guests_.push_back(std::move(guest));
    tree_.clusters_[cid].nodes.push_back(nid);

    // The guest pointer is bound in build(); store null for now
    nodes.emplace_back(std::move(parentNids), std::move(pages), nullptr, cid);

    return nid;
}

ClusterTreeBuilder::ClusterId ClusterTreeBuilder::createCluster(const ClusterId parentCid)
{
    auto &clusters = tree_.clusters_;
    const ClusterId cid = clusters.size();

    clusters.emplace_back(parentCid, std::vector<NodeId>{});
    clusters[parentCid].children.push_back(cid);

    return cid;
}

void ClusterTreeBuilder::setLabel(std::string label)
{
    label_ = std::move(label);
}

ClusterTree ClusterTreeBuilder::build() &&
{
    auto instance = std::make_shared<const Instance>(capacity_, std::move(guests_), std::move(label_));
    const auto instanceGuests = instance->guests();

    for (size_t i = 0; i < tree_.leaves_.size(); ++i) {
        tree_.nodes_[tree_.leaves_[i]].guest = &instanceGuests[i];
    }

    tree_.instance_ = std::move(instance);
    return std::move(tree_);
}

ClusterTreeBuilder::ClusterId ClusterTreeBuilder::rootCluster()
{
    return ClusterTree::rootCluster();
}

}  // namespace vmp
