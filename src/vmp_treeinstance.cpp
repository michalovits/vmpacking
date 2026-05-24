#include <vmp_treeinstance.h>

#include <cassert>
#include <queue>

namespace vmp
{

TreeInstance::TreeInstance(const size_t capacity, std::unordered_set<int> rootPages)
    : capacity_(capacity)
{
    nodes_ = std::vector<std::optional<Node>>(ROOT_NODE + 1);
    nodes_[ROOT_NODE] = Node(ROOT_NODE, std::move(rootPages), {});
}

size_t TreeInstance::addInnerNode(const size_t parent, std::unordered_set<int> pages)
{
    assert(parent < nodes_.size());
    assert(nodes_[parent].has_value());

    const size_t n = nodes_.size();
    const auto node = Node(parent, std::move(pages), {});

    nodes_.push_back(node);
    nodes_[parent]->children.push_back(n);

    return n;
}

size_t TreeInstance::addLeafNode(size_t parent, const std::shared_ptr<const Guest> &guest,
                                 std::unordered_set<int> pages)
{
    assert(parent < nodes_.size());
    assert(nodes_[parent].has_value());

    const size_t n = nodes_.size();
    const auto node = Node(parent, std::move(pages), { guest });

    leaves_.push_back(n);
    nodes_.push_back(node);

    nodes_[parent]->children.push_back(n);

    while (parent != ROOT_NODE) {
        nodes_[parent]->guests.insert(guest);
        parent = nodes_[parent]->parent;
    }
    nodes_[ROOT_NODE]->guests.insert(guest);

    return n;
}

const std::vector<size_t> &TreeInstance::childrenOfNode(const size_t node) const
{
    return nodes_[node]->children;
}

size_t TreeInstance::parentOfNode(const size_t node) const
{
    return nodes_[node]->parent;
}

const std::unordered_set<int> &TreeInstance::pagesOfNode(const size_t node) const
{
    return nodes_[node]->pages;
}

std::shared_ptr<const Guest> TreeInstance::guestOfNode(const size_t node) const
{
    assert(nodes_[node]->guests.size() == 1);

    return *nodes_[node]->guests.begin();
}

const std::unordered_set<std::shared_ptr<const Guest>> &
TreeInstance::guestsOfSubtree(const size_t root) const
{
    assert(root < nodes_.size());
    assert(nodes_[root].has_value());

    return nodes_[root]->guests;
}

bool TreeInstance::isLeafNode(const size_t node) const
{
    assert(node < nodes_.size());
    assert(nodes_[node].has_value());

    return nodes_[node]->children.empty();
}

size_t TreeInstance::nodeCount() const
{
    return nodes_.size();
}

size_t TreeInstance::capacity() const
{
    return capacity_;
}

const std::unordered_set<std::shared_ptr<const Guest>> &TreeInstance::guests() const
{
    assert(nodes_[ROOT_NODE].has_value());

    return nodes_[ROOT_NODE]->guests;
}

const std::vector<size_t> &TreeInstance::leafNodes() const
{
    return leaves_;
}

void TreeInstance::eraseSubtree(const size_t root)
{
    assert(root < nodes_.size());
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

size_t TreeInstance::rootNode()
{
    return ROOT_NODE;
}

}  // namespace vmp