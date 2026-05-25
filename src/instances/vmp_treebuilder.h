#ifndef VMP_TREEINSTANCEBUILDER_H
#define VMP_TREEINSTANCEBUILDER_H

#include <vmp_guest.h>
#include <vmp_treetopology.h>

#include <memory>
#include <unordered_set>

namespace vmp
{

class TreeBuilder
{
  public:
    TreeBuilder(size_t capacity, std::unordered_set<int> rootPages);

    size_t addInnerNode(size_t parent, std::unordered_set<int> pages);
    size_t addLeafNode(size_t parent, const std::shared_ptr<const Guest> &guest,
                       std::unordered_set<int> pages);

    [[nodiscard]] TreeInstance build() &&;

    static size_t rootNode();

  private:
    size_t capacity_;
    TreeTopology topology_;
};

}  // namespace vmp

#endif  // VMP_TREEINSTANCEBUILDER_H
