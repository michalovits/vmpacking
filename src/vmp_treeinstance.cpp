#include <vmp_treeinstance.h>

#include <cassert>
#include <queue>

namespace vmp
{

TreeInstance::TreeInstance(const size_t capacity, std::unordered_set<int> rootPages)
    : capacity(capacity)
{
    nodes = std::vector<std::optional<Node>>(ROOT_NODE + 1);
    nodes[ROOT_NODE] =
        Node(ROOT_NODE, std::move(rootPages), std::unordered_set<std::shared_ptr<const Guest>>{});
}

TreeInstance::TreeInstance(const size_t capacity, std::unordered_set<int> rootPages,
                           const std::shared_ptr<const Guest> &rootGuest)
    : capacity(capacity)
{
    nodes = std::vector<std::optional<Node>>(ROOT_NODE + 1);
    nodes[ROOT_NODE] = Node(ROOT_NODE, std::move(rootPages), std::unordered_set{ rootGuest });
}

size_t TreeInstance::addInner(const size_t parent, std::unordered_set<int> pages)
{
    const size_t newNode = nodes.size();
    nodes.push_back(std::make_optional<Node>(parent, std::move(pages),
                                             std::unordered_set<std::shared_ptr<const Guest>>{}));
    nodes[parent]->children.push_back(newNode);
    return newNode;
}

size_t TreeInstance::addLeaf(size_t parent, const std::shared_ptr<const Guest> &guest,
                             std::unordered_set<int> pages)
{
    const size_t newNode = nodes.size();
    nodes.push_back(
        std::make_optional<Node>(parent, std::move(pages), std::unordered_set{ guest }));
    leaves.push_back(newNode);
    nodes[parent]->children.push_back(newNode);

    while (parent != ROOT_NODE) {
        nodes[parent]->guests.insert(guest);
        parent = nodes[parent]->parent;
    }
    nodes[ROOT_NODE]->guests.insert(guest);

    return newNode;
}

const std::vector<size_t> &TreeInstance::getNodeChildren(const size_t node) const
{
    return nodes[node]->children;
}

size_t TreeInstance::getNodeParent(const size_t node) const
{
    return nodes[node]->parent;
}

const std::unordered_set<int> &TreeInstance::getNodePages(const size_t node) const
{
    return nodes[node]->pages;
}

std::shared_ptr<const Guest> TreeInstance::getNodeGuest(const size_t node) const
{
    assert(nodes[node]->guests.size() == 1);
    return *nodes[node]->guests.begin();
}

const std::unordered_set<std::shared_ptr<const Guest>> &
TreeInstance::getSubtreeGuests(const size_t root) const
{
    return nodes[root]->guests;
}

bool TreeInstance::nodeIsLeaf(const size_t node) const
{
    return nodes[node]->children.empty();
}

size_t TreeInstance::getNodeCount() const
{
    return nodes.size();
}

size_t TreeInstance::getCapacity() const
{
    return capacity;
}

const std::unordered_set<std::shared_ptr<const Guest>> &TreeInstance::getGuests() const
{
    return nodes[ROOT_NODE]->guests;
}

const std::vector<size_t> &TreeInstance::getLeaves() const
{
    return leaves;
}

void TreeInstance::removeSubtree(const size_t root)
{
    const auto &guestsToRemove = nodes[root]->guests;
    for (int node = 0; node < nodes.size(); ++node) {
        if (node == root || !nodes[node].has_value()) {
            continue;
        }
        for (const auto &guest : guestsToRemove) {
            nodes[node]->guests.erase(guest);
        }
    }

    std::queue<size_t> nodesToRemove;
    nodesToRemove.push(root);

    while (!nodesToRemove.empty()) {
        const size_t node = nodesToRemove.front();
        nodesToRemove.pop();

        for (size_t child : nodes[node]->children) {
            nodesToRemove.push(child);
        }

        const size_t parent = nodes[node]->parent;
        auto &parentChildren = nodes[parent]->children;
        const auto it = std::ranges::find(parentChildren, node);
        if (it != parentChildren.end()) {
            parentChildren.erase(it);
        }

        nodes[node].reset();
    }
}

size_t TreeInstance::getRootNode()
{
    return ROOT_NODE;
}

}  // namespace vmp