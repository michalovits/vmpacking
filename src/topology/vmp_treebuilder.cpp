#include <vmp_treebuilder.h>

#include <cassert>
#include <memory>
#include <utility>

namespace vmp
{

TreeBuilder::TreeBuilder(const size_t capacity, std::unordered_set<int> rootPages)
    : capacity_(capacity), tree_(std::move(rootPages))
{
}

TreeBuilder::NodeId TreeBuilder::addInnerNode(const NodeId parentNid, std::unordered_set<int> pages)
{
    auto &nodes = tree_.nodes_;
    assert(nodes[parentNid].has_value());

    const NodeId nid = nodes.size();
    nodes.push_back(Tree::Node(parentNid, std::move(pages), {}));
    nodes[parentNid]->children.push_back(nid);

    return nid;
}

TreeBuilder::NodeId TreeBuilder::addLeafNode(const NodeId parentNid, Guest guest,
                                             std::unordered_set<int> pages)
{
    auto &nodes = tree_.nodes_;
    assert(nodes[parentNid].has_value());

    const NodeId nid = nodes.size();
    tree_.leaves_.push_back(nid);
    guests_.push_back(std::move(guest));

    // Cache left empty until we build and finalise the guest allocations
    nodes.push_back(Tree::Node(parentNid, std::move(pages), {}));
    nodes[parentNid]->children.push_back(nid);

    return nid;
}

void TreeBuilder::setLabel(std::string label)
{
    label_ = std::move(label);
}

Tree TreeBuilder::build() &&
{
    auto instance =
        std::make_shared<const Instance>(capacity_, std::move(guests_), std::move(label_));
    const auto instanceGuests = instance->guests();

    auto &nodes = tree_.nodes_;

    for (size_t i = 0; i < tree_.leaves_.size(); ++i) {
        const NodeId leaf = tree_.leaves_[i];
        const Guest &guest = instanceGuests[i];

        nodes[leaf]->guests.insert(&guest);

        for (NodeId ancestor = nodes[leaf]->parent;; ancestor = nodes[ancestor]->parent) {
            nodes[ancestor]->guests.insert(&guest);
            if (ancestor == Tree::ROOT_NODE) {
                break;
            }
        }
    }

    tree_.instance_ = std::move(instance);
    return std::move(tree_);
}

TreeBuilder::NodeId TreeBuilder::rootNode()
{
    return Tree::rootNode();
}

}  // namespace vmp
