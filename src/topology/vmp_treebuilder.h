#ifndef VMP_TREEBUILDER_H
#define VMP_TREEBUILDER_H

#include <vmp_guest.h>
#include <vmp_tree.h>

#include <unordered_set>
#include <vector>

namespace vmp
{

class TreeBuilder
{
  public:
    using NodeId = Tree::NodeId;

    TreeBuilder(size_t capacity, std::unordered_set<int> rootPages);

    NodeId addInnerNode(NodeId parentNid, std::unordered_set<int> pages);
    NodeId addLeafNode(NodeId parentNid, Guest guest, std::unordered_set<int> pages);

    static NodeId rootNode();

    [[nodiscard]] Tree build() &&;

  private:
    size_t capacity_;
    Tree instance_;

    // Parallel to instance_.leaves_
    std::vector<Guest> guests_;
};

}  // namespace vmp

#endif  // VMP_TREEBUILDER_H
