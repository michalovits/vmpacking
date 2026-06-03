#ifndef VMP_TREE_H
#define VMP_TREE_H

#include <vmp_guest.h>
#include <vmp_instance.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

namespace vmp
{

class Tree
{
    friend class TreeBuilder;

  public:
    using NodeId = size_t;

    [[nodiscard]] std::span<const Guest> guests() const;
    [[nodiscard]] size_t guestCount() const;
    [[nodiscard]] size_t capacity() const;
    [[nodiscard]] const std::string &label() const;

    [[nodiscard]] const std::vector<NodeId> &childrenOfNode(NodeId nid) const;
    [[nodiscard]] NodeId parentOfNode(NodeId nid) const;

    [[nodiscard]] const std::unordered_set<int> &pagesOfNode(NodeId nid) const;
    [[nodiscard]] const Guest *guestOfNode(NodeId nid) const;
    [[nodiscard]] const std::unordered_set<const Guest *> &guestsOfSubtree(NodeId rootNid) const;
    [[nodiscard]] size_t nodeCount() const;

    [[nodiscard]] const std::vector<NodeId> &leafNodes() const;
    [[nodiscard]] bool isLeafNode(NodeId nid) const;

    /// Removes the subtree rooted at `rootNid`.
    /// Removes references to the subtree's guests from every ancestor's guest cache.
    ///
    /// Dangerous: does not rebuild the tree. Some inner nodes may now be redundant
    /// (contain pages that belong to only one child).
    void eraseSubtree(NodeId rootNid);

    static NodeId rootNode();

  private:
    explicit Tree(std::unordered_set<int> rootPages);

    struct Node
    {
        NodeId parent;
        std::vector<NodeId> children;

        // If it's an inner node, the pages shared by all descendants
        // If it's a leaf, the pages unique to the node
        std::unordered_set<int> pages;

        // Cache the guests of the subtree rooted here
        std::unordered_set<const Guest *> guests;

        Node(const NodeId parent, std::unordered_set<int> pages,
             std::unordered_set<const Guest *> guests)
            : parent(parent), pages(std::move(pages)), guests(std::move(guests))
        {
        }
    };

    std::shared_ptr<const Instance> instance_;
    std::vector<std::optional<Node>> nodes_;
    std::vector<NodeId> leaves_;

    static constexpr NodeId ROOT_NODE = 0;
};

}  // namespace vmp

#endif  // VMP_TREE_H
