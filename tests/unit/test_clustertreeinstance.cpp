#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_clustertreeinstance.h>

using namespace vmp;
using vmp::testing::makeGuest;

TEST_CASE("ClusterTreeInstance initialisation", "[clustertreeinstance]")
{
    const ClusterTreeInstance tree(8);

    CHECK(tree.capacity() == 8);
    CHECK(tree.nodeCount() == 0);
    CHECK(tree.clusterCount() == 1);
    CHECK(tree.leafNodes().empty());
    CHECK(tree.nodesOfCluster(ClusterTreeInstance::rootCluster()).empty());
}

TEST_CASE("ClusterTreeInstance::addInner", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::rootCluster();

    const size_t node = tree.addInnerNode(root, {}, { 1, 2 });

    CHECK(tree.nodeCount() == 1);
    CHECK(tree.nodeCountOfCluster(root) == 1);
    CHECK(tree.nodesOfCluster(root) == std::vector{ node });
    CHECK(tree.pagesOfNode(node) == std::unordered_set{ 1, 2 });
    CHECK(tree.parentsOfNode(node).empty());
    CHECK(tree.childrenOfNode(node).empty());
    CHECK_FALSE(tree.isLeafNode(node));
}

TEST_CASE("ClusterTreeInstance::addLeaf", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::rootCluster();
    const size_t parentNode = tree.addInnerNode(root, {}, { 1 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = tree.addLeafNode({ parentNode }, g, { 5 });

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

TEST_CASE("ClusterTreeInstance::addInner multi-parent", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::rootCluster();
    const size_t parentA = tree.addInnerNode(root, {}, { 1 });
    const size_t parentB = tree.addInnerNode(root, {}, { 2 });

    const size_t childCluster = tree.createCluster(root);
    const size_t child = tree.addInnerNode(childCluster, { parentA, parentB }, { 3 });

    CHECK(tree.parentsOfNode(child) == std::vector{ parentA, parentB });
    CHECK(tree.childrenOfNode(parentA) == std::vector{ child });
    CHECK(tree.childrenOfNode(parentB) == std::vector{ child });
    CHECK(tree.nodesOfCluster(childCluster) == std::vector{ child });
    CHECK(tree.pagesOfNode(child) == std::unordered_set{ 3 });
    CHECK_FALSE(tree.isLeafNode(child));
}

TEST_CASE("ClusterTreeInstance::addLeaf multi-parent", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::rootCluster();
    const size_t parentA = tree.addInnerNode(root, {}, { 1 });
    const size_t parentB = tree.addInnerNode(root, {}, { 2 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = tree.addLeafNode({ parentA, parentB }, g, { 3 });

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

TEST_CASE("ClusterTreeInstance::addLeaf gives each sibling leaf its own cluster",
          "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::rootCluster();
    const size_t parent = tree.addInnerNode(root, {}, { 1 });

    const auto g1 = makeGuest({ 2 });
    const auto g2 = makeGuest({ 3 });
    const size_t leaf1 = tree.addLeafNode({ parent }, g1, { 2 });
    const size_t leaf2 = tree.addLeafNode({ parent }, g2, { 3 });

    REQUIRE(tree.clusterCount() == 3);
    CHECK(tree.childrenOfCluster(root).size() == 2);
    CHECK(tree.childrenOfNode(parent) == std::vector{ leaf1, leaf2 });
    CHECK(tree.isLeafNode(leaf1));
    CHECK(tree.isLeafNode(leaf2));
}

TEST_CASE("ClusterTreeInstance::addInner deep", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t rootCluster = ClusterTreeInstance::rootCluster();
    const size_t innerA = tree.addInnerNode(rootCluster, {}, { 1 });

    const size_t childCluster = tree.createCluster(rootCluster);
    const size_t innerB = tree.addInnerNode(childCluster, { innerA }, { 2 });

    const size_t grandchildCluster = tree.createCluster(childCluster);
    const size_t innerC = tree.addInnerNode(grandchildCluster, { innerB }, { 3 });

    CHECK(tree.clusterCount() == 3);
    CHECK(tree.parentOfCluster(childCluster) == rootCluster);
    CHECK(tree.parentOfCluster(grandchildCluster) == childCluster);
    CHECK(tree.parentsOfNode(innerB) == std::vector{ innerA });
    CHECK(tree.parentsOfNode(innerC) == std::vector{ innerB });
    CHECK(tree.childrenOfNode(innerA) == std::vector{ innerB });
    CHECK(tree.childrenOfNode(innerB) == std::vector{ innerC });
}

TEST_CASE("ClusterTreeInstance::guests", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::rootCluster();
    const size_t parent = tree.addInnerNode(root, {}, { 1 });

    const auto g1 = makeGuest({ 2 });
    const auto g2 = makeGuest({ 3 });
    tree.addLeafNode({ parent }, g1, { 2 });
    tree.addLeafNode({ parent }, g2, { 3 });

    const auto guests = tree.guests();
    REQUIRE(guests.size() == 2);
    CHECK(((guests[0] == g1 && guests[1] == g2) || (guests[0] == g2 && guests[1] == g1)));
}

TEST_CASE("ClusterTreeInstance::createCluster", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::rootCluster();

    const size_t child = tree.createCluster(root);

    CHECK(tree.clusterCount() == 2);
    CHECK(tree.parentOfCluster(child) == root);
    CHECK(tree.childrenOfCluster(root) == std::vector{ child });
}
