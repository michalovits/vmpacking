#include <vmp_tree.h>

#include <cassert>
#include <queue>

namespace vmp
{

Tree::Tree(std::unordered_set<int> rootPages)
{
    nodes_ = std::vector<std::optional<Node>>(ROOT_NODE + 1);
    nodes_[ROOT_NODE] = Node(ROOT_NODE, std::move(rootPages), {});
}

std::span<const Guest *const> Tree::guests() const
{
    return instance_->guests();
}

size_t Tree::guestCount() const
{
    return instance_->guestCount();
}

size_t Tree::capacity() const
{
    return instance_->capacity();
}

const std::vector<Tree::NodeId> &Tree::childrenOfNode(const NodeId nid) const
{
    return nodes_[nid]->children;
}

Tree::NodeId Tree::parentOfNode(const NodeId nid) const
{
    return nodes_[nid]->parent;
}

const std::unordered_set<int> &Tree::pagesOfNode(const NodeId nid) const
{
    return nodes_[nid]->pages;
}

const Guest *Tree::guestOfNode(const NodeId nid) const
{
    assert(nodes_[nid]->guests.size() == 1);

    return *nodes_[nid]->guests.begin();
}

const std::unordered_set<const Guest *> &Tree::guestsOfSubtree(const NodeId rootNid) const
{
    assert(nodes_[rootNid].has_value());
    return nodes_[rootNid]->guests;
}

bool Tree::isLeafNode(const NodeId nid) const
{
    assert(nodes_[nid].has_value());
    return nodes_[nid]->children.empty();
}

size_t Tree::nodeCount() const
{
    return nodes_.size();
}

const std::vector<Tree::NodeId> &Tree::leafNodes() const
{
    return leaves_;
}

void Tree::eraseSubtree(const NodeId rootNid)
{
    assert(nodes_[rootNid].has_value());

    // Clean up references to these guests by just iterating over all nodes
    auto &guestsToRemove = nodes_[rootNid]->guests;

    for (NodeId nid = 0; nid < nodes_.size(); ++nid) {
        if (!nodes_[nid].has_value()) {
            continue;
        }
        if (nid == rootNid) {
            // we'll handle self after finishing the loop, as guestsToRemove is ref
            continue;
        }
        for (const auto &guest : guestsToRemove) {
            nodes_[nid]->guests.erase(guest);
        }
    }
    guestsToRemove.clear();

    // Detach subtree
    std::erase(nodes_[nodes_[rootNid]->parent]->children, rootNid);

    // Iterate subtree and clear each node
    std::queue<NodeId> nodesToRemove;
    nodesToRemove.push(rootNid);

    while (!nodesToRemove.empty()) {
        const NodeId nid = nodesToRemove.front();
        nodesToRemove.pop();

        for (const NodeId child : nodes_[nid]->children) {
            nodesToRemove.push(child);
        }
        nodes_[nid].reset();
    }
}

Tree::NodeId Tree::rootNode()
{
    return ROOT_NODE;
}

}  // namespace vmp
