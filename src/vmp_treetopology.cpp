#include <vmp_treetopology.h>

#include <cassert>
#include <queue>

namespace vmp
{

TreeTopology::TreeTopology(std::unordered_set<int> rootPages)
{
    nodes_ = std::vector<std::optional<Node>>(ROOT_NODE + 1);
    nodes_[ROOT_NODE] = Node(ROOT_NODE, std::move(rootPages), {});
}

const std::vector<size_t> &TreeTopology::childrenOfNode(const size_t node) const
{
    return nodes_[node]->children;
}

size_t TreeTopology::parentOfNode(const size_t node) const
{
    return nodes_[node]->parent;
}

const std::unordered_set<int> &TreeTopology::pagesOfNode(const size_t node) const
{
    return nodes_[node]->pages;
}

std::shared_ptr<const Guest> TreeTopology::guestOfNode(const size_t node) const
{
    assert(nodes_[node]->guests.size() == 1);

    return *nodes_[node]->guests.begin();
}

const std::unordered_set<std::shared_ptr<const Guest>> &
TreeTopology::guestsOfSubtree(const size_t root) const
{
    assert(nodes_[root].has_value());
    return nodes_[root]->guests;
}

bool TreeTopology::isLeafNode(const size_t node) const
{
    assert(nodes_[node].has_value());
    return nodes_[node]->children.empty();
}

size_t TreeTopology::nodeCount() const
{
    return nodes_.size();
}

const std::unordered_set<std::shared_ptr<const Guest>> &TreeTopology::guests() const
{
    assert(nodes_[ROOT_NODE].has_value());
    return nodes_[ROOT_NODE]->guests;
}

size_t TreeTopology::guestCount() const
{
    assert(nodes_[ROOT_NODE].has_value());
    return nodes_[ROOT_NODE]->guests.size();
}

const std::vector<size_t> &TreeTopology::leafNodes() const
{
    return leaves_;
}

void TreeTopology::eraseSubtree(const size_t root)
{
    assert(nodes_[root].has_value());

    // Clean up references to these guests by just iterating over all nodes
    auto &guestsToRemove = nodes_[root]->guests;

    for (size_t node = 0; node < nodes_.size(); ++node) {
        if (!nodes_[node].has_value()) {
            continue;
        }
        if (node == root) {
            // we'll handle self after finishing the loop, as guestsToRemove is ref
            continue;
        }
        for (const auto &guest : guestsToRemove) {
            nodes_[node]->guests.erase(guest);
        }
    }
    guestsToRemove.clear();

    // Detach subtree
    std::erase(nodes_[nodes_[root]->parent]->children, root);

    // Iterate subtree and clear each node
    std::queue<size_t> nodesToRemove;
    nodesToRemove.push(root);

    while (!nodesToRemove.empty()) {
        const size_t node = nodesToRemove.front();
        nodesToRemove.pop();

        for (const size_t child : nodes_[node]->children) {
            nodesToRemove.push(child);
        }
        nodes_[node].reset();
    }
}

size_t TreeTopology::rootNode()
{
    return ROOT_NODE;
}

}  // namespace vmp
