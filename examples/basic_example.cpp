#include <vmp_clustertreeinstance.h>
#include <vmp_generalinstance.h>
#include <vmp_solvers.h>
#include <vmp_treeinstance.h>

#include <iostream>

const size_t capacity = 4;

// A 4-page capacity makes possible a 2-host packing for these guests:
const auto guest1 = std::make_shared<const vmp::Guest>(std::unordered_set{ 1 });
const auto guest2 = std::make_shared<const vmp::Guest>(std::unordered_set{ 2, 3, 5, 8 });
const auto guest3 = std::make_shared<const vmp::Guest>(std::unordered_set{ 1 });
const auto guest4 = std::make_shared<const vmp::Guest>(std::unordered_set{ 3, 5 });
const auto guest5 = std::make_shared<const vmp::Guest>(std::unordered_set{ 6, 8 });

vmp::GeneralInstance mkGeneral();
vmp::TreeInstance mkTree();

void printResult(const vmp::Packing &packing);

int main()
{
    const auto general = mkGeneral();
    const auto tree = mkTree();

    // Using the general solvers
    printResult(vmp::solveByNextFit(general));
    printResult(vmp::solveByFirstFit(general));
    printResult(vmp::solveByEfficiency(general));
    printResult(vmp::solveByOpportunityAwareEfficiency(general));
    printResult(vmp::solveByOverloadAndRemove(general));

    // Using the tree solver with an intermediate solver which iterates over an std::unordered_set
    // collection of guests
    using SetGuestIt = std::unordered_set<std::shared_ptr<const vmp::Guest>>::const_iterator;

    printResult(vmp::solveByTree<SetGuestIt>(tree, vmp::proceedByFirstFit));
    printResult(vmp::solveByTree<SetGuestIt>(tree, vmp::proceedByOverloadAndRemove));

    // Using the general solvers on an instance ordered by tree insertion
    printResult(vmp::solveByOpportunityAwareEfficiency(tree));
    printResult(vmp::solveByOverloadAndRemove(tree));
}

vmp::GeneralInstance mkGeneral()
{
    return { capacity, { guest1, guest2, guest3, guest4, guest5 } };
}

vmp::TreeInstance mkTree()
{
    auto tree = vmp::TreeInstance(capacity, {});

    // TreeInstance only validates tree structure, not VM Packing invariants.
    // E.g. a page appearing in both an ancestor and descendant will not be detected.
    const size_t n = tree.addInner(vmp::TreeInstance::getRootNode(), {});
    const size_t left = tree.addInner(n, { 1 });
    tree.addLeaf(left, guest1, {});  // the "1" page is removed now
    tree.addLeaf(left, guest3, {});

    const size_t right = tree.addInner(n, { 3, 5 });
    tree.addLeaf(right, guest2, { 2, 8 });
    tree.addLeaf(right, guest4, {});

    tree.addLeaf(n, guest5, { 6, 8 });

    return tree;
}

void printResult(const vmp::Packing &packing)
{
    std::cout << packing.getHostCount() << std::endl;
}
