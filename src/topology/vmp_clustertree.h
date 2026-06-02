#ifndef VMP_CLUSTERTREE_H
#define VMP_CLUSTERTREE_H

#include <vmp_guest.h>
#include <vmp_instance.h>

#include <cstddef>
#include <memory>
#include <span>
#include <unordered_set>
#include <vector>

namespace vmp
{

class ClusterTree
{
    friend class ClusterTreeBuilder;

  public:
    using NodeId = size_t;
    using ClusterId = size_t;

    [[nodiscard]] std::span<const Guest *const> guests() const;
    [[nodiscard]] size_t guestCount() const;
    [[nodiscard]] size_t capacity() const;

    [[nodiscard]] const std::vector<NodeId> &nodesOfCluster(ClusterId cid) const;
    [[nodiscard]] const std::vector<ClusterId> &childrenOfCluster(ClusterId cid) const;
    [[nodiscard]] ClusterId parentOfCluster(ClusterId cid) const;
    [[nodiscard]] size_t nodeCountOfCluster(ClusterId cid) const;

    [[nodiscard]] const std::vector<NodeId> &parentsOfNode(NodeId nid) const;
    [[nodiscard]] const std::vector<NodeId> &childrenOfNode(NodeId nid) const;
    [[nodiscard]] const std::unordered_set<int> &pagesOfNode(NodeId nid) const;
    [[nodiscard]] const Guest *guestOfNode(NodeId nid) const;

    [[nodiscard]] size_t nodeCount() const;
    [[nodiscard]] const std::vector<NodeId> &leafNodes() const;

    [[nodiscard]] size_t clusterCount() const;
    [[nodiscard]] static ClusterId rootCluster();

    [[nodiscard]] bool isLeafCluster(ClusterId cid) const;
    [[nodiscard]] bool isLeafNode(NodeId nid) const;

    static constexpr ClusterId ROOT_CLUSTER = 0;

  private:
    ClusterTree();

    struct Node
    {
        // Parents must belong to the same cluster
        std::vector<NodeId> parents;
        std::vector<NodeId> children;

        // If it's an inner node, the pages shared by all descendants
        // If it's a leaf, the pages unique to the node
        std::unordered_set<int> pages;

        const Guest *guest;
        ClusterId cluster;

        Node(std::vector<NodeId> parents, std::unordered_set<int> pages, const Guest *guest,
             const ClusterId cluster)
            : parents(std::move(parents)), pages(std::move(pages)), guest(guest), cluster(cluster)
        {
        }
    };

    struct Cluster
    {
        ClusterId parent;
        std::vector<ClusterId> children;

        std::vector<NodeId> nodes;

        Cluster(const ClusterId parent, std::vector<NodeId> nodes)
            : parent(parent), nodes(std::move(nodes))
        {
        }

        Cluster() : parent(ROOT_CLUSTER), nodes({}) {}
    };

    std::shared_ptr<const Instance> instance_;
    std::vector<Node> nodes_;
    std::vector<NodeId> leaves_;
    std::vector<Cluster> clusters_;
};

}  // namespace vmp

#endif  // VMP_CLUSTERTREE_H
