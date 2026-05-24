#ifndef VMP_TREEINSTANCE_H
#define VMP_TREEINSTANCE_H

#include <vmp_guest.h>

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace vmp
{

class TreeInstance
{
    friend class TreeInstanceParser;

  public:
    size_t addInner(size_t parent, std::unordered_set<int> pages);
    size_t addLeaf(size_t parent, const std::shared_ptr<const Guest> &guest,
                   std::unordered_set<int> pages);

    [[nodiscard]] const std::vector<size_t> &getNodeChildren(size_t node) const;
    [[nodiscard]] size_t getNodeParent(size_t node) const;
    [[nodiscard]] const std::unordered_set<int> &getNodePages(size_t node) const;
    [[nodiscard]] std::shared_ptr<const Guest> getNodeGuest(size_t node) const;
    [[nodiscard]] const std::unordered_set<std::shared_ptr<const Guest>> &
    getSubtreeGuests(size_t root) const;
    [[nodiscard]] size_t getNodeCount() const;
    [[nodiscard]] size_t getCapacity() const;
    [[nodiscard]] const std::unordered_set<std::shared_ptr<const Guest>> &getGuests() const;
    [[nodiscard]] const std::vector<size_t> &getLeaves() const;

    [[nodiscard]] bool nodeIsLeaf(size_t node) const;

    /// Removes the subtree rooted at `root`.
    /// Removes references to the subtree's guests from every ancestor's guest cache.
    ///
    /// Dangerous: does not rebuild the tree. Some inner nodes may now be redundant
    /// (contain pages that belong to only one child).
    void forceDropSubtree(size_t root);

    static size_t getRootNode();

    TreeInstance(size_t capacity, std::unordered_set<int> rootPages);

  private:
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

    std::vector<std::optional<Node>> nodes;
    std::vector<size_t> leaves;

    const size_t capacity;
    static constexpr size_t ROOT_NODE = 0;
};

}  // namespace vmp

#endif  // VMP_TREEINSTANCE_H
