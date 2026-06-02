#include <vmp_instance.h>
#include <vmp_solvers.h>
#include <vmp_treebuilder.h>

#include <iostream>

const size_t capacity = 4;

// A 4-page capacity makes possible a 2-host packing for these guests:
const auto g1 = vmp::Guest({ 1 });
const auto g2 = vmp::Guest({ 2, 3, 5, 8 });
const auto g3 = vmp::Guest({ 1 });
const auto g4 = vmp::Guest({ 3, 5 });
const auto g5 = vmp::Guest({ 6, 8 });

vmp::Instance makeInstance()
{
    return vmp::Instance(capacity, { g1, g2, g3, g4, g5 });
}

vmp::Tree makeTree()
{
    auto builder = vmp::TreeBuilder(capacity, {});

    // The tree only validates tree structure, not VM Packing invariants.
    // E.g. a page appearing in both an ancestor and descendant will not be detected.
    const size_t inner = builder.addInnerNode(builder.rootNode(), {});
    const size_t left = builder.addInnerNode(inner, { 1 });
    const size_t right = builder.addInnerNode(inner, { 3, 5 });

    builder.addLeafNode(left, g1, {});  // the "1" page is removed now
    builder.addLeafNode(left, g3, {});
    builder.addLeafNode(right, g2, { 2, 8 });
    builder.addLeafNode(right, g4, {});
    builder.addLeafNode(inner, g5, { 6, 8 });

    return std::move(builder).build();
}

int main()
{
    const auto show = [](const vmp::Packing &p) {
        std::cout << p.hostCount() << std::endl;
    };

    const auto instance = makeInstance();
    const auto tree = makeTree();

    // Using the general solvers
    show(vmp::solveByNextFit(instance));
    show(vmp::solveByFirstFit(instance));
    show(vmp::solveByEfficiency(instance));
    show(vmp::solveByOpportunityAwareEfficiency(instance));
    show(vmp::solveByOverloadAndRemove(instance));

    // Using the tree solver requires an intermediate solver
    using SetGuestIt = std::unordered_set<const vmp::Guest *>::const_iterator;

    show(vmp::solveByTree<SetGuestIt>(tree, vmp::proceedByFirstFit));
    show(vmp::solveByTree<SetGuestIt>(tree, vmp::proceedByOverloadAndRemove));

    // Using the general solvers on the tree-shaped instance (ignoring structure)
    show(vmp::solveByOpportunityAwareEfficiency(tree));
    show(vmp::solveByOverloadAndRemove(tree));
}
