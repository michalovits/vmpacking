#include <vmp_solverutils.h>

#include <vmp_guest.h>
#include <vmp_host.h>

#include <cmath>
#include <queue>
#include <stdexcept>
#include <unordered_map>

namespace vmp
{

double calculateRelSize(const Guest &guest, const std::unordered_map<int, int> &pageFreq)
{
    double total = 0.0;

    for (int page : guest.pages) {
        auto pageIt = pageFreq.find(page);
        const int frequency = (pageIt != pageFreq.end()) ? pageIt->second : 0;
        total += (frequency > 0) ? (1.0 / frequency) : 1.0;
    }

    return total;
}

double calculateSizeRelRatio(const Guest &guest, const std::unordered_map<int, int> &pageFreq)
{
    return static_cast<double>(guest.uniquePageCount()) / calculateRelSize(guest, pageFreq);
}

double calculateOpportunityAwareEfficiency(const Guest &guest, const Host &host,
                                           const std::vector<std::unique_ptr<Host>> &allHosts)
{
    const size_t pagesOnHost = guest.countUniquePagesOn(host);

    size_t minDifferenceWithOtherHost = std::numeric_limits<size_t>::max();
    for (const auto &otherHost : allHosts) {
        if (&host != otherHost.get()) {
            minDifferenceWithOtherHost =
                std::min(minDifferenceWithOtherHost, otherHost->countPagesNotOn(guest));
        }
    }
    // Not found
    if (minDifferenceWithOtherHost == std::numeric_limits<size_t>::max()) {
        minDifferenceWithOtherHost = 0;
    }

    return static_cast<double>(pagesOnHost + minDifferenceWithOtherHost) /
           std::sqrt(guest.uniquePageCount());
}

std::unordered_map<Tree::NodeId, TreeLowerBounds>
calculateAllSubtreeLowerBounds(const Tree &tree, const size_t capacity)
{
    using NodeId = Tree::NodeId;

    std::unordered_map<NodeId, TreeLowerBounds> res;

    // The capacity of each of those hosts (as we go deeper down the tree, we
    // subtract from this capacity measurement to reflect that the nodes we've
    // visited must have been packed)
    std::unordered_map<NodeId, size_t> capacities;

    // For later bottom-up traversal
    std::unordered_map<NodeId, size_t> unvisitedChildCounts;

    std::queue<NodeId> bottomUpNodesToVisit;
    std::queue<NodeId> topDownNodesToVisit;

    // To calculate capacities we must go top-down
    const NodeId rootNid = tree.rootNode();

    topDownNodesToVisit.push(rootNid);
    capacities[rootNid] = capacity;

    while (!topDownNodesToVisit.empty()) {
        const NodeId nid = topDownNodesToVisit.front();
        topDownNodesToVisit.pop();

        const size_t weight = tree.pagesOfNode(nid).size();

        if (weight > capacities[nid]) {
            throw std::invalid_argument("calculateAllSubtreeLowerBounds: malformed Tree -- "
                                        "node page count exceeds capacity after ancestors");
        }

        const auto &children = tree.childrenOfNode(nid);
        for (const NodeId child : children) {
            capacities[child] = capacities[nid] - weight;
            topDownNodesToVisit.push(child);
        }
        if ((unvisitedChildCounts[nid] = children.size()) == 0) {
            bottomUpNodesToVisit.push(nid);
        }
    }

    // To calculate size and count we must go bottom-up
    while (!bottomUpNodesToVisit.empty()) {
        const NodeId nid = bottomUpNodesToVisit.front();
        bottomUpNodesToVisit.pop();

        const NodeId parent = tree.parentOfNode(nid);
        if (nid != rootNid && --unvisitedChildCounts[parent] == 0) {
            bottomUpNodesToVisit.push(parent);
        }

        const size_t weight = tree.pagesOfNode(nid).size();

        if (tree.isLeafNode(nid)) {
            res.emplace(nid, TreeLowerBounds(weight, 1));
            continue;
        }

        size_t childrenTotalSize = 0;
        for (const NodeId child : tree.childrenOfNode(nid)) {
            childrenTotalSize += res.at(child).size;
        }

        if (weight >= capacities[nid]) {
            throw std::invalid_argument("calculateAllSubtreeLowerBounds: malformed Tree -- "
                                        "internal node leaves no capacity for its subtree");
        }

        const size_t count = std::ceil(static_cast<double>(childrenTotalSize) /
                                       static_cast<double>(capacities[nid] - weight));
        const size_t size = childrenTotalSize + count * weight;

        res.emplace(nid, TreeLowerBounds(size, count));
    }

    return res;
}

}  // namespace vmp
