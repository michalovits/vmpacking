#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_treebuilder.h>

using namespace vmp;
using vmp::testing::makeGuest;

TEST_CASE("Empty TreeInstance initialisation", "[treeinstance]")
{
    const auto instance = TreeBuilder(8, { 7, 8 }).build();
    const auto &tree = instance.topology();

    CHECK(instance.capacity() == 8);

    CHECK(tree.nodeCount() == 1);  // sentinel/root node
    CHECK(tree.pagesOfNode(tree.rootNode()) == std::unordered_set{ 7, 8 });
    CHECK(tree.guests().empty());
    CHECK(tree.leafNodes().empty());
    CHECK(tree.isLeafNode(tree.rootNode()));
}

TEST_CASE("TreeBuilder::addInner", "[treeinstance]")
{
    TreeBuilder builder(8, { 1 });
    const auto root = builder.rootNode();

    const size_t inner = builder.addInnerNode(root, { 2, 3 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.nodeCount() == 2);
    CHECK(tree.parentOfNode(inner) == root);
    CHECK(tree.childrenOfNode(root) == std::vector{ inner });
    CHECK(tree.pagesOfNode(inner) == std::unordered_set{ 2, 3 });
    CHECK(tree.isLeafNode(inner));
    CHECK_FALSE(tree.isLeafNode(root));
}

TEST_CASE("TreeBuilder::addLeaf", "[treeinstance]")
{
    TreeBuilder builder(8, { 1 });
    const auto root = builder.rootNode();
    const size_t inner = builder.addInnerNode(root, { 2 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = builder.addLeafNode(inner, g, { 3 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.leafNodes() == std::vector{ leaf });
    CHECK(tree.guestOfNode(leaf) == g);
    CHECK(tree.guestsOfSubtree(leaf).contains(g));
    CHECK(tree.guestsOfSubtree(inner).contains(g));
    CHECK(tree.guestsOfSubtree(root).contains(g));
    CHECK(tree.guests().contains(g));
}

TEST_CASE("TreeBuilder::addInner nested", "[treeinstance]")
{
    TreeBuilder builder(8, { 1 });
    const auto root = builder.rootNode();

    const size_t innerA = builder.addInnerNode(root, { 2 });
    const size_t innerB = builder.addInnerNode(innerA, { 3 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.nodeCount() == 3);
    CHECK(tree.parentOfNode(innerA) == root);
    CHECK(tree.parentOfNode(innerB) == innerA);
    CHECK(tree.childrenOfNode(root) == std::vector{ innerA });
    CHECK(tree.childrenOfNode(innerA) == std::vector{ innerB });
    CHECK(tree.isLeafNode(innerB));
}

TEST_CASE("TreeBuilder::addLeaf bookkeeping at ancestors", "[treeinstance]")
{
    //   -- root --
    //   |        |
    // innerA   innerC
    //   |
    // innerB
    //   |
    //  leaf
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t innerA = builder.addInnerNode(root, { 2 });
    const size_t innerB = builder.addInnerNode(innerA, { 3 });
    const size_t innerC = builder.addInnerNode(root, { 4 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = builder.addLeafNode(innerB, g, { 6 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.guestsOfSubtree(leaf).contains(g));
    CHECK(tree.guestsOfSubtree(innerB).contains(g));
    CHECK(tree.guestsOfSubtree(innerA).contains(g));
    CHECK(tree.guestsOfSubtree(root).contains(g));
    CHECK_FALSE(tree.guestsOfSubtree(innerC).contains(g));
}

TEST_CASE("TreeBuilder::addLeaf accumulates sibling guests at the parent", "[treeinstance]")
{
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t inner = builder.addInnerNode(root, { 2 });

    const auto g1 = makeGuest({ 10 });
    const auto g2 = makeGuest({ 20 });
    const size_t leaf1 = builder.addLeafNode(inner, g1, { 3 });
    const size_t leaf2 = builder.addLeafNode(inner, g2, { 4 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.childrenOfNode(inner) == std::vector{ leaf1, leaf2 });
    CHECK(tree.leafNodes() == std::vector{ leaf1, leaf2 });
    CHECK(tree.guestsOfSubtree(inner).size() == 2);
    CHECK(tree.guestsOfSubtree(inner).contains(g1));
    CHECK(tree.guestsOfSubtree(inner).contains(g2));
    CHECK(tree.guests().size() == 2);
}

TEST_CASE("TreeTopology::eraseSubtree drops node and guests", "[treeinstance]")
{
    //   -- root --
    //   |        |
    // innerA   innerB
    //   |        |
    // leafA    leafB
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t innerA = builder.addInnerNode(root, { 2 });
    const size_t innerB = builder.addInnerNode(root, { 3 });

    const auto guestA = makeGuest({ 10 });
    const auto guestB = makeGuest({ 20 });
    builder.addLeafNode(innerA, guestA, { 4 });
    builder.addLeafNode(innerB, guestB, { 5 });

    const auto instance = std::move(builder).build();
    auto tree = instance.topology();

    REQUIRE(tree.guests().size() == 2);

    tree.eraseSubtree(innerA);

    CHECK(tree.guests().size() == 1);
    CHECK(tree.guests().contains(guestB));
    CHECK_FALSE(tree.guests().contains(guestA));
    CHECK(tree.childrenOfNode(root) == std::vector{ innerB });
}

TEST_CASE("TreeTopology::eraseSubtree of a leaf", "[treeinstance]")
{
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t inner = builder.addInnerNode(root, { 2 });

    const auto gA = makeGuest({ 10 });
    const auto gB = makeGuest({ 20 });
    const size_t leafA = builder.addLeafNode(inner, gA, { 3 });
    const size_t leafB = builder.addLeafNode(inner, gB, { 4 });

    const auto instance = std::move(builder).build();
    auto tree = instance.topology();

    tree.eraseSubtree(leafA);

    CHECK(tree.guests().size() == 1);
    CHECK(tree.guests().contains(gB));
    CHECK_FALSE(tree.guests().contains(gA));
    CHECK(tree.guestsOfSubtree(inner).size() == 1);
    CHECK(tree.childrenOfNode(inner) == std::vector{ leafB });
}

TEST_CASE("TreeTopology::eraseSubtree deep", "[treeinstance]")
{
    //   root
    //     |
    //   inner1
    //     |
    //   inner2
    //     |
    //   leaf (g)
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t inner1 = builder.addInnerNode(root, { 2 });
    const size_t inner2 = builder.addInnerNode(inner1, { 3 });

    const auto g = makeGuest({ 99 });
    builder.addLeafNode(inner2, g, { 4 });

    const auto instance = std::move(builder).build();
    auto tree = instance.topology();

    tree.eraseSubtree(inner1);

    CHECK(tree.guests().empty());
    CHECK(tree.guestsOfSubtree(root).empty());
    CHECK(tree.childrenOfNode(root).empty());
}

TEST_CASE("TreeTopology::eraseSubtree repeated", "[treeinstance]")
{
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t innerA = builder.addInnerNode(root, { 2 });
    const size_t innerB = builder.addInnerNode(root, { 3 });

    const auto gA = makeGuest({ 10 });
    const auto gB = makeGuest({ 20 });

    builder.addLeafNode(innerA, gA, { 4 });
    builder.addLeafNode(innerB, gB, { 5 });

    const auto instance = std::move(builder).build();
    auto tree = instance.topology();

    tree.eraseSubtree(innerA);
    tree.eraseSubtree(innerB);

    CHECK(tree.guests().empty());
    CHECK(tree.childrenOfNode(root).empty());
}

TEST_CASE("TreeTopology::eraseSubtree preserves page sharing", "[treeinstance]")
{
    // after dropping leafA, the walk up from leafB should reconstruct its page set correctly
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t inner = builder.addInnerNode(root, { 2 });

    const auto gA = makeGuest({ 1, 2, 3 });
    const auto gB = makeGuest({ 1, 2, 4 });

    const size_t leafA = builder.addLeafNode(inner, gA, { 3 });
    const size_t leafB = builder.addLeafNode(inner, gB, { 4 });

    const auto instance = std::move(builder).build();
    auto tree = instance.topology();

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

TEST_CASE("TreeTopology::eraseSubtree does not affect copies", "[treeinstance]")
{
    TreeBuilder builder(8, { 1 });

    const size_t inner = builder.addInnerNode(builder.rootNode(), { 2 });
    const auto g = makeGuest({ 3 });

    builder.addLeafNode(inner, g, { 4 });

    const auto original = std::move(builder).build();

    auto workingTree = original.topology();
    workingTree.eraseSubtree(inner);

    CHECK(workingTree.guests().empty());
    CHECK(original.topology().guests().contains(g));
}
