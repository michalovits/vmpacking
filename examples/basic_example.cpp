#include <vmp_generaltopology.h>
#include <vmp_solvers.h>
#include <vmp_treeinstancebuilder.h>

#include <iostream>

const size_t capacity = 4;

// A 4-page capacity makes possible a 2-host packing for these guests:
const auto guest1 = std::make_shared<const vmp::Guest>(std::unordered_set{ 1 });
const auto guest2 = std::make_shared<const vmp::Guest>(std::unordered_set{ 2, 3, 5, 8 });
const auto guest3 = std::make_shared<const vmp::Guest>(std::unordered_set{ 1 });
const auto guest4 = std::make_shared<const vmp::Guest>(std::unordered_set{ 3, 5 });
const auto guest5 = std::make_shared<const vmp::Guest>(std::unordered_set{ 6, 8 });

vmp::GeneralInstance makeGeneralInstance();
vmp::TreeInstance makeTreeInstance();

int main()
{
    const auto out = [](const vmp::Packing &p) {
        std::cout << p.hostCount() << std::endl;
    };

    const auto g = makeGeneralInstance();
    const auto t = makeTreeInstance();

    // Using the general solvers
    out(vmp::solveByNextFit(g));
    out(vmp::solveByFirstFit(g));
    out(vmp::solveByEfficiency(g));
    out(vmp::solveByOpportunityAwareEfficiency(g));
    out(vmp::solveByOverloadAndRemove(g));

    // Using the tree solver requires an intermediate solver
    using SetGuestIt = std::unordered_set<std::shared_ptr<const vmp::Guest>>::const_iterator;

    out(vmp::solveByTree<SetGuestIt>(t, vmp::proceedByFirstFit));
    out(vmp::solveByTree<SetGuestIt>(t, vmp::proceedByOverloadAndRemove));

    // Using the general solvers on an instance ordered by tree insertion
    out(vmp::solveByOpportunityAwareEfficiency(t));
    out(vmp::solveByOverloadAndRemove(t));
}

vmp::GeneralInstance makeGeneralInstance()
{
    return vmp::GeneralInstance(capacity, std::vector{ guest1, guest2, guest3, guest4, guest5 });
}

vmp::TreeInstance makeTreeInstance()
{
    auto builder = vmp::TreeInstanceBuilder(capacity, {});

    // The topology only validates tree structure, not VM Packing invariants.
    // E.g. a page appearing in both an ancestor and descendant will not be detected.
    const size_t inner = builder.addInnerNode(builder.rootNode(), {});
    const size_t left = builder.addInnerNode(inner, { 1 });
    const size_t right = builder.addInnerNode(inner, { 3, 5 });

    builder.addLeafNode(left, guest1, {});  // the "1" page is removed now
    builder.addLeafNode(left, guest3, {});
    builder.addLeafNode(right, guest2, { 2, 8 });
    builder.addLeafNode(right, guest4, {});
    builder.addLeafNode(inner, guest5, { 6, 8 });

    return std::move(builder).build();
}
