#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <instance_builders.h>
#include <vmp_clustertreebuilder.h>

using namespace vmp;
using vmp::testing::makeGuest;

TEST_CASE("ClusterTreeInstance initialisation", "[clustertreeinstance]")
{
    const auto instance = ClusterTreeBuilder(8).build();
    const auto &tree = instance.topology();

    CHECK(instance.capacity() == 8);

    CHECK(tree.nodeCount() == 0);
    CHECK(tree.clusterCount() == 1);
    CHECK(tree.leafNodes().empty());
    CHECK(tree.nodesOfCluster(tree.rootCluster()).empty());
}

TEST_CASE("ClusterTreeBuilder::addInner", "[clustertreeinstance]")
{
    ClusterTreeBuilder builder(8);

    const size_t root = builder.rootCluster();
    const size_t node = builder.addInnerNode(root, {}, { 1, 2 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.nodeCount() == 1);
    CHECK(tree.nodeCountOfCluster(root) == 1);
    CHECK(tree.nodesOfCluster(root) == std::vector{ node });
    CHECK(tree.pagesOfNode(node) == std::unordered_set{ 1, 2 });
    CHECK(tree.parentsOfNode(node).empty());
    CHECK(tree.childrenOfNode(node).empty());
    CHECK_FALSE(tree.isLeafNode(node));
}

TEST_CASE("ClusterTreeBuilder::addLeaf", "[clustertreeinstance]")
{
    ClusterTreeBuilder builder(8);
    const size_t root = builder.rootCluster();
    const size_t parentNode = builder.addInnerNode(root, {}, { 1 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = builder.addLeafNode({ parentNode }, g, { 5 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.isLeafNode(leaf));
    CHECK(tree.guestOfNode(leaf) == g);
    CHECK(tree.leafNodes() == std::vector{ leaf });
    CHECK(tree.parentsOfNode(leaf) == std::vector{ parentNode });
    CHECK(tree.childrenOfNode(parentNode) == std::vector{ leaf });

    REQUIRE(tree.clusterCount() == 2);
    const size_t leafCluster = 1;
    CHECK(tree.parentOfCluster(leafCluster) == root);
    CHECK(tree.nodesOfCluster(leafCluster) == std::vector{ leaf });
    CHECK(tree.isLeafCluster(leafCluster));
}

TEST_CASE("ClusterTreeBuilder::addInner multi-parent", "[clustertreeinstance]")
{
    ClusterTreeBuilder builder(8);
    const size_t root = builder.rootCluster();
    const size_t parentA = builder.addInnerNode(root, {}, { 1 });
    const size_t parentB = builder.addInnerNode(root, {}, { 2 });

    const size_t childCluster = builder.createCluster(root);
    const size_t child = builder.addInnerNode(childCluster, { parentA, parentB }, { 3 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.parentsOfNode(child) == std::vector{ parentA, parentB });
    CHECK(tree.childrenOfNode(parentA) == std::vector{ child });
    CHECK(tree.childrenOfNode(parentB) == std::vector{ child });
    CHECK(tree.nodesOfCluster(childCluster) == std::vector{ child });
    CHECK(tree.pagesOfNode(child) == std::unordered_set{ 3 });
    CHECK_FALSE(tree.isLeafNode(child));
}

TEST_CASE("ClusterTreeBuilder::addLeaf multi-parent", "[clustertreeinstance]")
{
    ClusterTreeBuilder builder(8);
    const size_t root = builder.rootCluster();
    const size_t parentA = builder.addInnerNode(root, {}, { 1 });
    const size_t parentB = builder.addInnerNode(root, {}, { 2 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = builder.addLeafNode({ parentA, parentB }, g, { 3 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.parentsOfNode(leaf) == std::vector{ parentA, parentB });
    CHECK(tree.childrenOfNode(parentA) == std::vector{ leaf });
    CHECK(tree.childrenOfNode(parentB) == std::vector{ leaf });
    CHECK(tree.isLeafNode(leaf));

    REQUIRE(tree.clusterCount() == 2);
    const size_t leafCluster = 1;

    // The new leaf cluster should appear under root exactly once despite two parents
    CHECK(tree.childrenOfCluster(root) == std::vector{ leafCluster });
    CHECK(tree.isLeafCluster(leafCluster));
}

TEST_CASE("ClusterTreeBuilder::addLeaf gives each sibling leaf its own cluster",
          "[clustertreeinstance]")
{
    ClusterTreeBuilder builder(8);
    const size_t root = builder.rootCluster();
    const size_t parent = builder.addInnerNode(root, {}, { 1 });

    const auto g1 = makeGuest({ 2 });
    const auto g2 = makeGuest({ 3 });

    const size_t leaf1 = builder.addLeafNode({ parent }, g1, { 2 });
    const size_t leaf2 = builder.addLeafNode({ parent }, g2, { 3 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    REQUIRE(tree.clusterCount() == 3);

    CHECK(tree.childrenOfCluster(root).size() == 2);
    CHECK(tree.childrenOfNode(parent) == std::vector{ leaf1, leaf2 });

    CHECK(tree.isLeafNode(leaf1));
    CHECK(tree.isLeafNode(leaf2));
}

TEST_CASE("ClusterTreeBuilder::addInner deep", "[clustertreeinstance]")
{
    ClusterTreeBuilder builder(8);
    const size_t rootCluster = builder.rootCluster();
    const size_t innerA = builder.addInnerNode(rootCluster, {}, { 1 });

    const size_t childCluster = builder.createCluster(rootCluster);
    const size_t innerB = builder.addInnerNode(childCluster, { innerA }, { 2 });

    const size_t grandchildCluster = builder.createCluster(childCluster);
    const size_t innerC = builder.addInnerNode(grandchildCluster, { innerB }, { 3 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.clusterCount() == 3);

    CHECK(tree.parentOfCluster(childCluster) == rootCluster);
    CHECK(tree.parentOfCluster(grandchildCluster) == childCluster);

    CHECK(tree.parentsOfNode(innerB) == std::vector{ innerA });
    CHECK(tree.parentsOfNode(innerC) == std::vector{ innerB });

    CHECK(tree.childrenOfNode(innerA) == std::vector{ innerB });
    CHECK(tree.childrenOfNode(innerB) == std::vector{ innerC });
}

TEST_CASE("ClusterTreeTopology::guests", "[clustertreeinstance]")
{
    ClusterTreeBuilder builder(8);

    const size_t root = builder.rootCluster();
    const size_t parent = builder.addInnerNode(root, {}, { 1 });

    const auto g1 = makeGuest({ 2 });
    const auto g2 = makeGuest({ 3 });
    builder.addLeafNode({ parent }, g1, { 2 });
    builder.addLeafNode({ parent }, g2, { 3 });

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    REQUIRE(tree.guestCount() == 2);
    CHECK_THAT(tree.guests(), Catch::Matchers::VectorContains(g1));
    CHECK_THAT(tree.guests(), Catch::Matchers::VectorContains(g2));
}

TEST_CASE("ClusterTreeBuilder::createCluster", "[clustertreeinstance]")
{
    ClusterTreeBuilder builder(8);

    const size_t root = builder.rootCluster();
    const size_t child = builder.createCluster(root);

    const auto instance = std::move(builder).build();
    const auto &tree = instance.topology();

    CHECK(tree.clusterCount() == 2);
    CHECK(tree.parentOfCluster(child) == root);
    CHECK(tree.childrenOfCluster(root) == std::vector{ child });
}
