#include <catch2/catch_test_macros.hpp>
#include <vmp_treebuilder.h>

#include <unordered_set>
#include <vector>

using namespace vmp;

TEST_CASE("Empty Tree initialisation", "[tree]")
{
    const auto tree = TreeBuilder(8, { 7, 8 }).build();

    CHECK(tree.capacity() == 8);

    CHECK(tree.nodeCount() == 1);  // sentinel/root node
    CHECK(tree.pagesOfNode(tree.rootNode()) == std::unordered_set{ 7, 8 });
    CHECK(tree.guestsOfSubtree(tree.rootNode()).empty());
    CHECK(tree.leafNodes().empty());
    CHECK(tree.isLeafNode(tree.rootNode()));
}

TEST_CASE("TreeBuilder::addInner", "[tree]")
{
    TreeBuilder builder(8, { 1 });
    const auto root = builder.rootNode();

    const size_t inner = builder.addInnerNode(root, { 2, 3 });

    const auto tree = std::move(builder).build();

    CHECK(tree.nodeCount() == 2);
    CHECK(tree.parentOfNode(inner) == root);
    CHECK(tree.childrenOfNode(root) == std::vector{ inner });
    CHECK(tree.pagesOfNode(inner) == std::unordered_set{ 2, 3 });
    CHECK(tree.isLeafNode(inner));
    CHECK_FALSE(tree.isLeafNode(root));
}

TEST_CASE("TreeBuilder::addLeaf", "[tree]")
{
    TreeBuilder builder(8, { 1 });
    const auto root = builder.rootNode();
    const size_t inner = builder.addInnerNode(root, { 2 });

    const size_t leaf = builder.addLeafNode(inner, Guest({ 5 }), { 3 });

    const auto tree = std::move(builder).build();

    const Guest *g = tree.guestOfNode(leaf);
    CHECK(tree.leafNodes() == std::vector{ leaf });
    CHECK(g->pages == std::unordered_set{ 5 });
    CHECK(tree.guestsOfSubtree(leaf).contains(g));
    CHECK(tree.guestsOfSubtree(inner).contains(g));
    CHECK(tree.guestsOfSubtree(root).contains(g));
}

TEST_CASE("TreeBuilder::addInner nested", "[tree]")
{
    TreeBuilder builder(8, { 1 });
    const auto root = builder.rootNode();

    const size_t innerA = builder.addInnerNode(root, { 2 });
    const size_t innerB = builder.addInnerNode(innerA, { 3 });

    const auto tree = std::move(builder).build();

    CHECK(tree.nodeCount() == 3);
    CHECK(tree.parentOfNode(innerA) == root);
    CHECK(tree.parentOfNode(innerB) == innerA);
    CHECK(tree.childrenOfNode(root) == std::vector{ innerA });
    CHECK(tree.childrenOfNode(innerA) == std::vector{ innerB });
    CHECK(tree.isLeafNode(innerB));
}

TEST_CASE("TreeBuilder::addLeaf bookkeeping at ancestors", "[tree]")
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

    const size_t leaf = builder.addLeafNode(innerB, Guest({ 5 }), { 6 });

    const auto tree = std::move(builder).build();

    const Guest *g = tree.guestOfNode(leaf);
    CHECK(tree.guestsOfSubtree(leaf).contains(g));
    CHECK(tree.guestsOfSubtree(innerB).contains(g));
    CHECK(tree.guestsOfSubtree(innerA).contains(g));
    CHECK(tree.guestsOfSubtree(root).contains(g));
    CHECK_FALSE(tree.guestsOfSubtree(innerC).contains(g));
}

TEST_CASE("TreeBuilder::addLeaf accumulates sibling guests at the parent", "[tree]")
{
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t inner = builder.addInnerNode(root, { 2 });

    const size_t leaf1 = builder.addLeafNode(inner, Guest({ 10 }), { 3 });
    const size_t leaf2 = builder.addLeafNode(inner, Guest({ 20 }), { 4 });

    const auto tree = std::move(builder).build();

    const Guest *g1 = tree.guestOfNode(leaf1);
    const Guest *g2 = tree.guestOfNode(leaf2);

    CHECK(tree.childrenOfNode(inner) == std::vector{ leaf1, leaf2 });
    CHECK(tree.leafNodes() == std::vector{ leaf1, leaf2 });
    CHECK(tree.guestsOfSubtree(inner).size() == 2);
    CHECK(tree.guestsOfSubtree(inner).contains(g1));
    CHECK(tree.guestsOfSubtree(inner).contains(g2));
    CHECK(tree.guestCount() == 2);
}

TEST_CASE("Tree::eraseSubtree drops node and guests", "[tree]")
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

    const size_t leafA = builder.addLeafNode(innerA, Guest({ 10 }), { 4 });
    const size_t leafB = builder.addLeafNode(innerB, Guest({ 20 }), { 5 });

    auto tree = std::move(builder).build();

    const Guest *guestA = tree.guestOfNode(leafA);
    const Guest *guestB = tree.guestOfNode(leafB);

    REQUIRE(tree.guestsOfSubtree(tree.rootNode()).size() == 2);

    tree.eraseSubtree(innerA);

    CHECK(tree.guestsOfSubtree(tree.rootNode()).size() == 1);
    CHECK(tree.guestsOfSubtree(tree.rootNode()).contains(guestB));
    CHECK_FALSE(tree.guestsOfSubtree(tree.rootNode()).contains(guestA));
    CHECK(tree.childrenOfNode(root) == std::vector{ innerB });
}

TEST_CASE("Tree::eraseSubtree of a leaf", "[tree]")
{
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t inner = builder.addInnerNode(root, { 2 });

    const size_t leafA = builder.addLeafNode(inner, Guest({ 10 }), { 3 });
    const size_t leafB = builder.addLeafNode(inner, Guest({ 20 }), { 4 });

    auto tree = std::move(builder).build();

    const Guest *gA = tree.guestOfNode(leafA);
    const Guest *gB = tree.guestOfNode(leafB);

    tree.eraseSubtree(leafA);

    CHECK(tree.guestsOfSubtree(tree.rootNode()).size() == 1);
    CHECK(tree.guestsOfSubtree(tree.rootNode()).contains(gB));
    CHECK_FALSE(tree.guestsOfSubtree(tree.rootNode()).contains(gA));
    CHECK(tree.guestsOfSubtree(inner).size() == 1);
    CHECK(tree.childrenOfNode(inner) == std::vector{ leafB });
}

TEST_CASE("Tree::eraseSubtree deep", "[tree]")
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

    builder.addLeafNode(inner2, Guest({ 99 }), { 4 });

    auto tree = std::move(builder).build();

    tree.eraseSubtree(inner1);

    CHECK(tree.guestsOfSubtree(tree.rootNode()).empty());
    CHECK(tree.guestsOfSubtree(root).empty());
    CHECK(tree.childrenOfNode(root).empty());
}

TEST_CASE("Tree::eraseSubtree repeated", "[tree]")
{
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t innerA = builder.addInnerNode(root, { 2 });
    const size_t innerB = builder.addInnerNode(root, { 3 });

    builder.addLeafNode(innerA, Guest({ 10 }), { 4 });
    builder.addLeafNode(innerB, Guest({ 20 }), { 5 });

    auto tree = std::move(builder).build();

    tree.eraseSubtree(innerA);
    tree.eraseSubtree(innerB);

    CHECK(tree.guestsOfSubtree(tree.rootNode()).empty());
    CHECK(tree.childrenOfNode(root).empty());
}

TEST_CASE("Tree::eraseSubtree preserves page sharing", "[tree]")
{
    // after dropping leafA, the walk up from leafB should reconstruct its page set correctly
    TreeBuilder builder(8, { 1 });

    const auto root = builder.rootNode();
    const size_t inner = builder.addInnerNode(root, { 2 });

    const size_t leafA = builder.addLeafNode(inner, Guest({ 1, 2, 3 }), { 3 });
    const size_t leafB = builder.addLeafNode(inner, Guest({ 1, 2, 4 }), { 4 });

    auto tree = std::move(builder).build();

    const Guest *gB = tree.guestOfNode(leafB);

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

TEST_CASE("Tree::eraseSubtree does not affect copies", "[tree]")
{
    TreeBuilder builder(8, { 1 });

    const size_t inner = builder.addInnerNode(builder.rootNode(), { 2 });
    const size_t leaf = builder.addLeafNode(inner, Guest({ 3 }), { 4 });

    const auto original = std::move(builder).build();
    const Guest *g = original.guestOfNode(leaf);

    auto workingTree = original;
    workingTree.eraseSubtree(inner);

    CHECK(workingTree.guestsOfSubtree(workingTree.rootNode()).empty());
    CHECK(original.guestsOfSubtree(original.rootNode()).contains(g));
}
