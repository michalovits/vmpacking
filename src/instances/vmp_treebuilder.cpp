#include <vmp_treebuilder.h>

#include <cassert>
#include <utility>

namespace vmp
{

TreeBuilder::TreeBuilder(const size_t capacity, std::unordered_set<int> rootPages)
    : capacity_(capacity), topology_(std::move(rootPages))
{
}

size_t TreeBuilder::addInnerNode(const size_t parent, std::unordered_set<int> pages)
{
    auto &nodes = topology_.nodes_;
    assert(nodes[parent].has_value());

    const size_t node = nodes.size();
    nodes.push_back(TreeTopology::Node(parent, std::move(pages), {}));
    nodes[parent]->children.push_back(node);

    return node;
}

size_t TreeBuilder::addLeafNode(size_t parent, const std::shared_ptr<const Guest> &guest,
                                std::unordered_set<int> pages)
{
    auto &nodes = topology_.nodes_;
    assert(nodes[parent].has_value());

    const size_t node = nodes.size();
    topology_.leaves_.push_back(node);
    nodes.push_back(TreeTopology::Node(parent, std::move(pages), { guest }));

    nodes[parent]->children.push_back(node);

    while (parent != TreeTopology::ROOT_NODE) {
        nodes[parent]->guests.insert(guest);
        parent = nodes[parent]->parent;
    }
    nodes[TreeTopology::ROOT_NODE]->guests.insert(guest);

    return node;
}

TreeInstance TreeBuilder::build() &&
{
    return TreeInstance(capacity_, std::move(topology_));
}

size_t TreeBuilder::rootNode()
{
    return TreeTopology::rootNode();
}

}  // namespace vmp
