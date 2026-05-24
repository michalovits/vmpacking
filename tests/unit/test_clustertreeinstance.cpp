#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_clustertreeinstance.h>

using namespace vmp;
using vmp::testing::makeGuest;

TEST_CASE("ClusterTreeInstance initialisation", "[clustertreeinstance]")
{
    const ClusterTreeInstance tree(8);

    CHECK(tree.getCapacity() == 8);
    CHECK(tree.getNodeCount() == 0);
    CHECK(tree.getClusterCount() == 1);
    CHECK(tree.getLeafNodes().empty());
    CHECK(tree.getClusterNodes(ClusterTreeInstance::getRootCluster()).empty());
}

TEST_CASE("ClusterTreeInstance::addInner", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::getRootCluster();

    const size_t node = tree.addInner(root, {}, { 1, 2 });

    CHECK(tree.getNodeCount() == 1);
    CHECK(tree.nodeCountOf(root) == 1);
    CHECK(tree.getClusterNodes(root) == std::vector{ node });
    CHECK(tree.getNodePages(node) == std::unordered_set{ 1, 2 });
    CHECK(tree.getNodeParents(node).empty());
    CHECK(tree.getNodeChildren(node).empty());
    CHECK_FALSE(tree.nodeIsLeaf(node));
}

TEST_CASE("ClusterTreeInstance::addLeaf", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::getRootCluster();
    const size_t parentNode = tree.addInner(root, {}, { 1 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = tree.addLeaf({ parentNode }, g, { 5 });

    CHECK(tree.nodeIsLeaf(leaf));
    CHECK(tree.getNodeGuest(leaf) == g);
    CHECK(tree.getLeafNodes() == std::vector{ leaf });
    CHECK(tree.getNodeParents(leaf) == std::vector{ parentNode });
    CHECK(tree.getNodeChildren(parentNode) == std::vector{ leaf });

    REQUIRE(tree.getClusterCount() == 2);
    const size_t leafCluster = 1;
    CHECK(tree.getClusterParent(leafCluster) == root);
    CHECK(tree.getClusterNodes(leafCluster) == std::vector{ leaf });
    CHECK(tree.clusterIsLeaf(leafCluster));
}

TEST_CASE("ClusterTreeInstance::addInner multi-parent", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::getRootCluster();
    const size_t parentA = tree.addInner(root, {}, { 1 });
    const size_t parentB = tree.addInner(root, {}, { 2 });

    const size_t childCluster = tree.createCluster(root);
    const size_t child = tree.addInner(childCluster, { parentA, parentB }, { 3 });

    CHECK(tree.getNodeParents(child) == std::vector{ parentA, parentB });
    CHECK(tree.getNodeChildren(parentA) == std::vector{ child });
    CHECK(tree.getNodeChildren(parentB) == std::vector{ child });
    CHECK(tree.getClusterNodes(childCluster) == std::vector{ child });
    CHECK(tree.getNodePages(child) == std::unordered_set{ 3 });
    CHECK_FALSE(tree.nodeIsLeaf(child));
}

TEST_CASE("ClusterTreeInstance::addLeaf multi-parent", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::getRootCluster();
    const size_t parentA = tree.addInner(root, {}, { 1 });
    const size_t parentB = tree.addInner(root, {}, { 2 });

    const auto g = makeGuest({ 5 });
    const size_t leaf = tree.addLeaf({ parentA, parentB }, g, { 3 });

    CHECK(tree.getNodeParents(leaf) == std::vector{ parentA, parentB });
    CHECK(tree.getNodeChildren(parentA) == std::vector{ leaf });
    CHECK(tree.getNodeChildren(parentB) == std::vector{ leaf });
    CHECK(tree.nodeIsLeaf(leaf));

    REQUIRE(tree.getClusterCount() == 2);
    const size_t leafCluster = 1;

    // The new leaf cluster should appear under root exactly once despite two parents
    CHECK(tree.getClusterChildren(root) == std::vector{ leafCluster });
    CHECK(tree.clusterIsLeaf(leafCluster));
}

TEST_CASE("ClusterTreeInstance::addLeaf gives each sibling leaf its own cluster",
          "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::getRootCluster();
    const size_t parent = tree.addInner(root, {}, { 1 });

    const auto g1 = makeGuest({ 2 });
    const auto g2 = makeGuest({ 3 });
    const size_t leaf1 = tree.addLeaf({ parent }, g1, { 2 });
    const size_t leaf2 = tree.addLeaf({ parent }, g2, { 3 });

    REQUIRE(tree.getClusterCount() == 3);
    CHECK(tree.getClusterChildren(root).size() == 2);
    CHECK(tree.getNodeChildren(parent) == std::vector{ leaf1, leaf2 });
    CHECK(tree.nodeIsLeaf(leaf1));
    CHECK(tree.nodeIsLeaf(leaf2));
}

TEST_CASE("ClusterTreeInstance::addInner deep", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t rootCluster = ClusterTreeInstance::getRootCluster();
    const size_t innerA = tree.addInner(rootCluster, {}, { 1 });

    const size_t childCluster = tree.createCluster(rootCluster);
    const size_t innerB = tree.addInner(childCluster, { innerA }, { 2 });

    const size_t grandchildCluster = tree.createCluster(childCluster);
    const size_t innerC = tree.addInner(grandchildCluster, { innerB }, { 3 });

    CHECK(tree.getClusterCount() == 3);
    CHECK(tree.getClusterParent(childCluster) == rootCluster);
    CHECK(tree.getClusterParent(grandchildCluster) == childCluster);
    CHECK(tree.getNodeParents(innerB) == std::vector{ innerA });
    CHECK(tree.getNodeParents(innerC) == std::vector{ innerB });
    CHECK(tree.getNodeChildren(innerA) == std::vector{ innerB });
    CHECK(tree.getNodeChildren(innerB) == std::vector{ innerC });
}

TEST_CASE("ClusterTreeInstance::getGuests", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::getRootCluster();
    const size_t parent = tree.addInner(root, {}, { 1 });

    const auto g1 = makeGuest({ 2 });
    const auto g2 = makeGuest({ 3 });
    tree.addLeaf({ parent }, g1, { 2 });
    tree.addLeaf({ parent }, g2, { 3 });

    const auto guests = tree.getGuests();
    REQUIRE(guests.size() == 2);
    CHECK(((guests[0] == g1 && guests[1] == g2) || (guests[0] == g2 && guests[1] == g1)));
}

TEST_CASE("ClusterTreeInstance::createCluster", "[clustertreeinstance]")
{
    ClusterTreeInstance tree(8);
    const size_t root = ClusterTreeInstance::getRootCluster();

    const size_t child = tree.createCluster(root);

    CHECK(tree.getClusterCount() == 2);
    CHECK(tree.getClusterParent(child) == root);
    CHECK(tree.getClusterChildren(root) == std::vector{ child });
}
