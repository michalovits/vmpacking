#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_treeinstance.h>

using namespace vmp;
using vmp::testing::makeGuest;

TEST_CASE("Empty TreeInstance initialisation", "[treeinstance]")
{
    const TreeInstance tree(8, { 7, 8 });

    CHECK(tree.capacity() == 8);
    CHECK(tree.nodeCount() == 1);  // sentinel/root node
    CHECK(tree.pagesOfNode(TreeInstance::rootNode()) == std::unordered_set{ 7, 8 });
    CHECK(tree.guests().empty());
    CHECK(tree.leafNodes().empty());
    CHECK(tree.isLeafNode(TreeInstance::rootNode()));
}

TEST_CASE("TreeInstance::addInner", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::rootNode();

    const size_t inner = tree.addInnerNode(root, { 2, 3 });

    CHECK(tree.nodeCount() == 2);
    CHECK(tree.parentOfNode(inner) == root);
    CHECK(tree.childrenOfNode(root) == std::vector{ inner });
    CHECK(tree.pagesOfNode(inner) == std::unordered_set{ 2, 3 });
    CHECK(tree.isLeafNode(inner));
    CHECK_FALSE(tree.isLeafNode(root));
}

TEST_CASE("TreeInstance::addLeaf", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::rootNode();
    const size_t inner = tree.addInnerNode(root, { 2 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = tree.addLeafNode(inner, g, { 3 });

    CHECK(tree.leafNodes() == std::vector{ leaf });
    CHECK(tree.guestOfNode(leaf) == g);
    CHECK(tree.guestsOfSubtree(leaf).contains(g));
    CHECK(tree.guestsOfSubtree(inner).contains(g));
    CHECK(tree.guestsOfSubtree(root).contains(g));
    CHECK(tree.guests().contains(g));
}

TEST_CASE("TreeInstance::addInner nested", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::rootNode();

    const size_t innerA = tree.addInnerNode(root, { 2 });
    const size_t innerB = tree.addInnerNode(innerA, { 3 });

    CHECK(tree.nodeCount() == 3);
    CHECK(tree.parentOfNode(innerA) == root);
    CHECK(tree.parentOfNode(innerB) == innerA);
    CHECK(tree.childrenOfNode(root) == std::vector{ innerA });
    CHECK(tree.childrenOfNode(innerA) == std::vector{ innerB });
    CHECK(tree.isLeafNode(innerB));
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
    const auto root = TreeInstance::rootNode();
    const size_t innerA = tree.addInnerNode(root, { 2 });
    const size_t innerB = tree.addInnerNode(innerA, { 3 });
    const size_t innerC = tree.addInnerNode(root, { 4 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = tree.addLeafNode(innerB, g, { 6 });

    CHECK(tree.guestsOfSubtree(leaf).contains(g));
    CHECK(tree.guestsOfSubtree(innerB).contains(g));
    CHECK(tree.guestsOfSubtree(innerA).contains(g));
    CHECK(tree.guestsOfSubtree(root).contains(g));
    CHECK_FALSE(tree.guestsOfSubtree(innerC).contains(g));
}

TEST_CASE("TreeInstance::addLeaf accumulates sibling guests at the parent", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::rootNode();
    const size_t inner = tree.addInnerNode(root, { 2 });

    const auto g1 = makeGuest({ 10 });
    const auto g2 = makeGuest({ 20 });
    const size_t leaf1 = tree.addLeafNode(inner, g1, { 3 });
    const size_t leaf2 = tree.addLeafNode(inner, g2, { 4 });

    CHECK(tree.childrenOfNode(inner) == std::vector{ leaf1, leaf2 });
    CHECK(tree.leafNodes() == std::vector{ leaf1, leaf2 });
    CHECK(tree.guestsOfSubtree(inner).size() == 2);
    CHECK(tree.guestsOfSubtree(inner).contains(g1));
    CHECK(tree.guestsOfSubtree(inner).contains(g2));
    CHECK(tree.guests().size() == 2);
}

TEST_CASE("TreeInstance::eraseSubtree drops node and guests", "[treeinstance]")
{
    //   -- root --
    //   |        |
    // innerA   innerB
    //   |        |
    // leafA    leafB
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::rootNode();
    const size_t innerA = tree.addInnerNode(root, { 2 });
    const size_t innerB = tree.addInnerNode(root, { 3 });

    const auto guestA = makeGuest({ 10 });
    const auto guestB = makeGuest({ 20 });
    tree.addLeafNode(innerA, guestA, { 4 });
    tree.addLeafNode(innerB, guestB, { 5 });

    REQUIRE(tree.guests().size() == 2);

    tree.eraseSubtree(innerA);

    CHECK(tree.guests().size() == 1);
    CHECK(tree.guests().contains(guestB));
    CHECK_FALSE(tree.guests().contains(guestA));
    CHECK(tree.childrenOfNode(root) == std::vector{ innerB });
}

TEST_CASE("TreeInstance::eraseSubtree of a leaf", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::rootNode();
    const size_t inner = tree.addInnerNode(root, { 2 });

    const auto gA = makeGuest({ 10 });
    const auto gB = makeGuest({ 20 });
    const size_t leafA = tree.addLeafNode(inner, gA, { 3 });
    const size_t leafB = tree.addLeafNode(inner, gB, { 4 });

    tree.eraseSubtree(leafA);

    CHECK(tree.guests().size() == 1);
    CHECK(tree.guests().contains(gB));
    CHECK_FALSE(tree.guests().contains(gA));
    CHECK(tree.guestsOfSubtree(inner).size() == 1);
    CHECK(tree.childrenOfNode(inner) == std::vector{ leafB });
}

TEST_CASE("TreeInstance::eraseSubtree deep", "[treeinstance]")
{
    //   root
    //     |
    //   inner1
    //     |
    //   inner2
    //     |
    //   leaf (g)
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::rootNode();
    const size_t inner1 = tree.addInnerNode(root, { 2 });
    const size_t inner2 = tree.addInnerNode(inner1, { 3 });

    const auto g = makeGuest({ 99 });
    tree.addLeafNode(inner2, g, { 4 });

    tree.eraseSubtree(inner1);

    CHECK(tree.guests().empty());
    CHECK(tree.guestsOfSubtree(root).empty());
    CHECK(tree.childrenOfNode(root).empty());
}

TEST_CASE("TreeInstance::eraseSubtree repeated", "[treeinstance]")
{
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::rootNode();
    const size_t innerA = tree.addInnerNode(root, { 2 });
    const size_t innerB = tree.addInnerNode(root, { 3 });

    const auto gA = makeGuest({ 10 });
    const auto gB = makeGuest({ 20 });
    tree.addLeafNode(innerA, gA, { 4 });
    tree.addLeafNode(innerB, gB, { 5 });

    tree.eraseSubtree(innerA);
    tree.eraseSubtree(innerB);

    CHECK(tree.guests().empty());
    CHECK(tree.childrenOfNode(root).empty());
}

TEST_CASE("TreeInstance::eraseSubtree preserves page sharing", "[treeinstance]")
{
    // after dropping leafA, the walk up from leafB should reconstruct its page set correctly
    TreeInstance tree(8, { 1 });
    const auto root = TreeInstance::rootNode();
    const size_t inner = tree.addInnerNode(root, { 2 });

    const auto gA = makeGuest({ 1, 2, 3 });
    const auto gB = makeGuest({ 1, 2, 4 });
    const size_t leafA = tree.addLeafNode(inner, gA, { 3 });
    const size_t leafB = tree.addLeafNode(inner, gB, { 4 });

    tree.eraseSubtree(leafA);

    std::unordered_set<int> pathPages;
    for (size_t node = leafB;; node = tree.parentOfNode(node)) {
        for (const int p : tree.pagesOfNode(node)) {
            pathPages.insert(p);
        }
        if (node == root) {
            break;
        }
    }
    CHECK(pathPages == gB->pages);
}

TEST_CASE("TreeInstance::eraseSubtree does not affect copies", "[treeinstance]")
{
    TreeInstance original(8, { 1 });
    const size_t inner = original.addInnerNode(TreeInstance::rootNode(), { 2 });
    const auto g = makeGuest({ 3 });
    original.addLeafNode(inner, g, { 4 });

    TreeInstance copy = original;
    copy.eraseSubtree(inner);

    CHECK(copy.guests().empty());
    CHECK(original.guests().contains(g));
}

TEST_CASE("TreeInstance copy", "[treeinstance]")
{
    TreeInstance original(8, { 1 });
    const size_t inner = original.addInnerNode(TreeInstance::rootNode(), { 2 });
    const auto g = makeGuest({ 3 });
    original.addLeafNode(inner, g, { 4 });

    TreeInstance copy = original;
    copy.eraseSubtree(inner);

    CHECK(copy.guests().empty());
    CHECK(original.guests().contains(g));
}
