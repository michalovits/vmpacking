#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_treeinstance.h>

using namespace vmp;
using vmp::testing::makeGuest;

TEST_CASE("Empty TreeInstance initialisation", "[treeinstance]")
{
    const TreeInstance tree(8, { 7, 8 });

    CHECK(tree.getCapacity() == 8);
    CHECK(tree.getNodeCount() == 1);  // sentinel/root node
    CHECK(tree.getNodePages(TreeInstance::getRootNode()) == std::unordered_set{ 7, 8 });
    CHECK(tree.getGuests().empty());
    CHECK(tree.getLeaves().empty());
    CHECK(tree.nodeIsLeaf(TreeInstance::getRootNode()));
}

TEST_CASE("TreeInstance::addInner", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();

    const size_t inner = tree.addInner(root, { 2, 3 });

    CHECK(tree.getNodeCount() == 2);
    CHECK(tree.getNodeParent(inner) == root);
    CHECK(tree.getNodeChildren(root) == std::vector{ inner });
    CHECK(tree.getNodePages(inner) == std::unordered_set{ 2, 3 });
    CHECK(tree.nodeIsLeaf(inner));
    CHECK_FALSE(tree.nodeIsLeaf(root));
}

TEST_CASE("TreeInstance::addLeaf", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();
    const size_t inner = tree.addInner(root, { 2 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = tree.addLeaf(inner, g, { 3 });

    CHECK(tree.getLeaves() == std::vector{ leaf });
    CHECK(tree.getNodeGuest(leaf) == g);
    CHECK(tree.getSubtreeGuests(leaf).contains(g));
    CHECK(tree.getSubtreeGuests(inner).contains(g));
    CHECK(tree.getSubtreeGuests(root).contains(g));
    CHECK(tree.getGuests().contains(g));
}

TEST_CASE("TreeInstance::addInner nested", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();

    const size_t innerA = tree.addInner(root, { 2 });
    const size_t innerB = tree.addInner(innerA, { 3 });

    CHECK(tree.getNodeCount() == 3);
    CHECK(tree.getNodeParent(innerA) == root);
    CHECK(tree.getNodeParent(innerB) == innerA);
    CHECK(tree.getNodeChildren(root) == std::vector{ innerA });
    CHECK(tree.getNodeChildren(innerA) == std::vector{ innerB });
    CHECK(tree.nodeIsLeaf(innerB));
}

TEST_CASE("TreeInstance::addLeaf bookkeeping at ancestors", "[treeinstance]")
{
    //   -- root --
    //   |        |
    // innerA   innerC
    //   |
    // innerB
    //   |
    //  leaf
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();
    const size_t innerA = tree.addInner(root, { 2 });
    const size_t innerB = tree.addInner(innerA, { 3 });
    const size_t innerC = tree.addInner(root, { 4 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = tree.addLeaf(innerB, g, { 6 });

    CHECK(tree.getSubtreeGuests(leaf).contains(g));
    CHECK(tree.getSubtreeGuests(innerB).contains(g));
    CHECK(tree.getSubtreeGuests(innerA).contains(g));
    CHECK(tree.getSubtreeGuests(root).contains(g));
    CHECK_FALSE(tree.getSubtreeGuests(innerC).contains(g));
}

TEST_CASE("TreeInstance::addLeaf accumulates sibling guests at the parent", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();
    const size_t inner = tree.addInner(root, { 2 });

    const auto g1 = makeGuest({ 10 });
    const auto g2 = makeGuest({ 20 });
    const size_t leaf1 = tree.addLeaf(inner, g1, { 3 });
    const size_t leaf2 = tree.addLeaf(inner, g2, { 4 });

    CHECK(tree.getNodeChildren(inner) == std::vector{ leaf1, leaf2 });
    CHECK(tree.getLeaves() == std::vector{ leaf1, leaf2 });
    CHECK(tree.getSubtreeGuests(inner).size() == 2);
    CHECK(tree.getSubtreeGuests(inner).contains(g1));
    CHECK(tree.getSubtreeGuests(inner).contains(g2));
    CHECK(tree.getGuests().size() == 2);
}

TEST_CASE("TreeInstance::forceDropSubtree drops node and guests", "[treeinstance]")
{
    //   -- root --
    //   |        |
    // innerA   innerB
    //   |        |
    // leafA    leafB
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();
    const size_t innerA = tree.addInner(root, { 2 });
    const size_t innerB = tree.addInner(root, { 3 });

    const auto guestA = makeGuest({ 10 });
    const auto guestB = makeGuest({ 20 });
    tree.addLeaf(innerA, guestA, { 4 });
    tree.addLeaf(innerB, guestB, { 5 });

    REQUIRE(tree.getGuests().size() == 2);

    tree.forceDropSubtree(innerA);

    CHECK(tree.getGuests().size() == 1);
    CHECK(tree.getGuests().contains(guestB));
    CHECK_FALSE(tree.getGuests().contains(guestA));
    CHECK(tree.getNodeChildren(root) == std::vector{ innerB });
}

TEST_CASE("TreeInstance::forceDropSubtree of a leaf", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();
    const size_t inner = tree.addInner(root, { 2 });

    const auto gA = makeGuest({ 10 });
    const auto gB = makeGuest({ 20 });
    const size_t leafA = tree.addLeaf(inner, gA, { 3 });
    const size_t leafB = tree.addLeaf(inner, gB, { 4 });

    tree.forceDropSubtree(leafA);

    CHECK(tree.getGuests().size() == 1);
    CHECK(tree.getGuests().contains(gB));
    CHECK_FALSE(tree.getGuests().contains(gA));
    CHECK(tree.getSubtreeGuests(inner).size() == 1);
    CHECK(tree.getNodeChildren(inner) == std::vector{ leafB });
}

TEST_CASE("TreeInstance::forceDropSubtree deep", "[treeinstance]")
{
    //   root
    //     |
    //   inner1
    //     |
    //   inner2
    //     |
    //   leaf (g)
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();
    const size_t inner1 = tree.addInner(root, { 2 });
    const size_t inner2 = tree.addInner(inner1, { 3 });

    const auto g = makeGuest({ 99 });
    tree.addLeaf(inner2, g, { 4 });

    tree.forceDropSubtree(inner1);

    CHECK(tree.getGuests().empty());
    CHECK(tree.getSubtreeGuests(root).empty());
    CHECK(tree.getNodeChildren(root).empty());
}

TEST_CASE("TreeInstance::forceDropSubtree repeated", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();
    const size_t innerA = tree.addInner(root, { 2 });
    const size_t innerB = tree.addInner(root, { 3 });

    const auto gA = makeGuest({ 10 });
    const auto gB = makeGuest({ 20 });
    tree.addLeaf(innerA, gA, { 4 });
    tree.addLeaf(innerB, gB, { 5 });

    tree.forceDropSubtree(innerA);
    tree.forceDropSubtree(innerB);

    CHECK(tree.getGuests().empty());
    CHECK(tree.getNodeChildren(root).empty());
}

TEST_CASE("TreeInstance::forceDropSubtree preserves page sharing", "[treeinstance]")
{
    // after dropping leafA, the walk up from leafB should reconstruct its page set correctly
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::getRootNode();
    const size_t inner = tree.addInner(root, { 2 });

    const auto gA = makeGuest({ 1, 2, 3 });
    const auto gB = makeGuest({ 1, 2, 4 });
    const size_t leafA = tree.addLeaf(inner, gA, { 3 });
    const size_t leafB = tree.addLeaf(inner, gB, { 4 });

    tree.forceDropSubtree(leafA);

    std::unordered_set<int> pathPages;
    for (size_t node = leafB;; node = tree.getNodeParent(node)) {
        for (const int p : tree.getNodePages(node)) {
            pathPages.insert(p);
        }
        if (node == root) {
            break;
        }
    }
    CHECK(pathPages == gB->pages);
}

TEST_CASE("TreeInstance copy", "[treeinstance]")
{
    TreeInstance original(8, { 1 });
    const size_t inner = original.addInner(TreeInstance::getRootNode(), { 2 });
    const auto g = makeGuest({ 3 });
    original.addLeaf(inner, g, { 4 });

    TreeInstance copy = original;
    copy.forceDropSubtree(inner);

    CHECK(copy.getGuests().empty());
    CHECK(original.getGuests().contains(g));
}
