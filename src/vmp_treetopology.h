#ifndef VMP_TREETOPOLOGY_H
#define VMP_TREETOPOLOGY_H

#include <vmp_guest.h>
#include <vmp_instance.h>

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace vmp
{

class TreeTopology
{
    friend class TreeInstanceBuilder;

  public:
    [[nodiscard]] const std::vector<size_t> &childrenOfNode(size_t node) const;
    [[nodiscard]] size_t parentOfNode(size_t node) const;

    [[nodiscard]] const std::unordered_set<int> &pagesOfNode(size_t node) const;
    [[nodiscard]] std::shared_ptr<const Guest> guestOfNode(size_t node) const;
    [[nodiscard]] const std::unordered_set<std::shared_ptr<const Guest>> &
    guestsOfSubtree(size_t root) const;
    [[nodiscard]] size_t nodeCount() const;

    [[nodiscard]] const std::vector<size_t> &leafNodes() const;
    [[nodiscard]] bool isLeafNode(size_t node) const;

    [[nodiscard]] const std::unordered_set<std::shared_ptr<const Guest>> &guests() const;
    [[nodiscard]] size_t guestCount() const;

    /// Removes the subtree rooted at `root`.
    /// Removes references to the subtree's guests from every ancestor's guest cache.
    ///
    /// Dangerous: does not rebuild the tree. Some inner nodes may now be redundant
    /// (contain pages that belong to only one child).
    void eraseSubtree(size_t root);

    static size_t rootNode();

  private:
    explicit TreeTopology(std::unordered_set<int> rootPages);

    struct Node
    {
        std::size_t parent;
        std::vector<size_t> children;

        // If it's an inner node, the pages shared by all descendants
        // If it's a leaf, the pages unique to the node
        std::unordered_set<int> pages;

        // Cache the guests of the subtree rooted here
        std::unordered_set<std::shared_ptr<const Guest>> guests;

        Node(const size_t parent, std::unordered_set<int> pages,
             std::unordered_set<std::shared_ptr<const Guest>> guests)
            : parent(parent), pages(std::move(pages)), guests(std::move(guests))
        {
        }
    };

    std::vector<std::optional<Node>> nodes_;
    std::vector<size_t> leaves_;
    static constexpr size_t ROOT_NODE = 0;
};

using TreeInstance = Instance<TreeTopology>;

}  // namespace vmp

#endif  // VMP_TREETOPOLOGY_H
