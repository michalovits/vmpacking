#ifndef VMP_CLUSTERTREETOPOLOGY_H
#define VMP_CLUSTERTREETOPOLOGY_H

#include <vmp_guest.h>
#include <vmp_instance.h>

#include <memory>
#include <unordered_set>
#include <vector>

namespace vmp
{

class ClusterTreeTopology
{
    friend class ClusterTreeBuilder;

  public:
    [[nodiscard]] const std::vector<size_t> &nodesOfCluster(size_t cluster) const;
    [[nodiscard]] const std::vector<size_t> &childrenOfCluster(size_t cluster) const;
    [[nodiscard]] size_t parentOfCluster(size_t cluster) const;
    [[nodiscard]] size_t nodeCountOfCluster(size_t cluster) const;

    [[nodiscard]] const std::vector<size_t> &parentsOfNode(size_t node) const;
    [[nodiscard]] const std::vector<size_t> &childrenOfNode(size_t node) const;
    [[nodiscard]] const std::unordered_set<int> &pagesOfNode(size_t node) const;
    [[nodiscard]] const std::shared_ptr<const Guest> &guestOfNode(size_t node) const;

    [[nodiscard]] size_t nodeCount() const;
    [[nodiscard]] const std::vector<size_t> &leafNodes() const;

    [[nodiscard]] size_t clusterCount() const;
    [[nodiscard]] static size_t rootCluster();

    [[nodiscard]] bool isLeafCluster(size_t cluster) const;
    [[nodiscard]] bool isLeafNode(size_t node) const;

    [[nodiscard]] std::vector<std::shared_ptr<const Guest>> guests() const;
    [[nodiscard]] size_t guestCount() const;

    static constexpr size_t ROOT_CLUSTER = 0;

  private:
    ClusterTreeTopology();

    struct Node
    {
        // Parents must be in the same cluster
        std::vector<size_t> parents;
        std::vector<size_t> children;

        // If it's an inner node, the pages shared by all descendants
        // If it's a leaf, the pages unique to the node
        std::unordered_set<int> pages;

        std::shared_ptr<const Guest> guest;
        size_t cluster;

        Node(std::vector<size_t> parents, std::unordered_set<int> pages,
             const std::shared_ptr<const Guest> &guest, const size_t cluster)
            : parents(std::move(parents)), pages(std::move(pages)), guest(guest), cluster(cluster)
        {
        }
    };

    struct Cluster
    {
        size_t parent;
        std::vector<size_t> children;

        std::vector<size_t> nodes;

        Cluster(const size_t parent, std::vector<size_t> nodes)
            : parent(parent), nodes(std::move(nodes))
        {
        }

        Cluster() : parent(ROOT_CLUSTER), nodes({}) {}
    };

    std::vector<Node> nodes_;
    std::vector<size_t> leaves_;
    std::vector<Cluster> clusters_;
};

using ClusterTreeInstance = Instance<ClusterTreeTopology>;

}  // namespace vmp

#endif  // VMP_CLUSTERTREETOPOLOGY_H
