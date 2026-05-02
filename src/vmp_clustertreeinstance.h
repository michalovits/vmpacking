#ifndef VMP_CLUSTERTREEINSTANCE_H
#define VMP_CLUSTERTREEINSTANCE_H

#include <vmp_guest.h>

#include <vector>

namespace vmp
{

class ClusterTreeInstance
{
    friend class ClusterTreeInstanceParser;

  public:
    size_t addInner(size_t cluster, std::vector<size_t> parents, std::unordered_set<int> pages);
    size_t addLeaf(std::vector<size_t> parents, const std::shared_ptr<const Guest> &guest,
                   std::unordered_set<int> pages);

    size_t createCluster(size_t parent);

    [[nodiscard]] const std::vector<size_t> &getClusterNodes(size_t cluster) const;
    [[nodiscard]] const std::vector<size_t> &getClusterChildren(size_t cluster) const;
    [[nodiscard]] size_t getClusterParent(size_t cluster) const;
    [[nodiscard]] bool clusterIsLeaf(size_t cluster) const;

    [[nodiscard]] const std::vector<size_t> &getNodeParents(size_t node) const;
    [[nodiscard]] const std::vector<size_t> &getNodeChildren(size_t node) const;
    [[nodiscard]] const std::unordered_set<int> &getNodePages(size_t node) const;
    [[nodiscard]] const std::shared_ptr<const Guest> &getNodeGuest(size_t node) const;
    [[nodiscard]] bool nodeIsLeaf(size_t node) const;
    [[nodiscard]] size_t getNodeCount() const;
    [[nodiscard]] size_t nodeCountOf(size_t cluster) const;

    [[nodiscard]] const std::vector<size_t> &getLeafNodes() const;
    [[nodiscard]] size_t getClusterCount() const;
    [[nodiscard]] static size_t getRootCluster();

    [[nodiscard]] std::vector<std::shared_ptr<const Guest>> getGuests() const;
    [[nodiscard]] size_t getCapacity() const;

    explicit ClusterTreeInstance(size_t capacity);

    static constexpr size_t ROOT_CLUSTER = 0;

  private:
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

    [[nodiscard]] bool checkNodesAreInCluster(const std::vector<size_t> &nodes,
                                              size_t cluster) const;

    std::vector<Node> nodes;
    std::vector<size_t> leaves;
    std::vector<Cluster> clusters;

    const size_t capacity;
};

}  // namespace vmp
#endif