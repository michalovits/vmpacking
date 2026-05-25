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

double calculateOpportunityAwareEfficiency(const Guest &guest, const std::shared_ptr<Host> &host,
                                           const std::vector<std::shared_ptr<Host>> &allHosts)
{
    const size_t pagesOnHost = guest.countUniquePagesOn(*host);

    size_t minDifferenceWithOtherHost = std::numeric_limits<size_t>::max();
    for (const auto &otherHost : allHosts) {
        if (host != otherHost) {
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

std::unordered_map<size_t, TreeLowerBounds> calculateAllSubtreeLowerBounds(const TreeTopology &tree,
                                                                           const size_t capacity)
{
    std::unordered_map<size_t, TreeLowerBounds> res;

    // The capacity of each of those hosts (as we go deeper down the tree, we
    // subtract from this capacity measurement to reflect that the nodes we've
    // visited must have been packed)
    std::unordered_map<size_t, size_t> capacities;

    // For later bottom-up traversal
    std::unordered_map<size_t, size_t> unvisitedChildCounts;

    std::queue<size_t> bottomUpNodesToVisit;
    std::queue<size_t> topDownNodesToVisit;

    // To calculate capacities we must go top-down
    const size_t root = tree.rootNode();

    topDownNodesToVisit.push(root);
    capacities[root] = capacity;

    while (!topDownNodesToVisit.empty()) {
        const size_t node = topDownNodesToVisit.front();
        topDownNodesToVisit.pop();

        const size_t weight = tree.pagesOfNode(node).size();

        if (weight > capacities[node]) {
            throw std::invalid_argument("calculateAllSubtreeLowerBounds: malformed TreeTopology -- "
                                        "node page count exceeds capacity after ancestors");
        }

        const auto &children = tree.childrenOfNode(node);
        for (const size_t child : children) {
            capacities[child] = capacities[node] - weight;
            topDownNodesToVisit.push(child);
        }
        if ((unvisitedChildCounts[node] = children.size()) == 0) {
            bottomUpNodesToVisit.push(node);
        }
    }

    // To calculate size and count we must go bottom-up
    while (!bottomUpNodesToVisit.empty()) {
        const size_t node = bottomUpNodesToVisit.front();
        bottomUpNodesToVisit.pop();

        const size_t parent = tree.parentOfNode(node);
        if (node != root && --unvisitedChildCounts[parent] == 0) {
            bottomUpNodesToVisit.push(parent);
        }

        const size_t weight = tree.pagesOfNode(node).size();

        if (tree.isLeafNode(node)) {
            res.emplace(node, TreeLowerBounds(weight, 1));
            continue;
        }

        size_t childrenTotalSize = 0;
        for (const size_t child : tree.childrenOfNode(node)) {
            childrenTotalSize += res.at(child).size;
        }

        if (weight >= capacities[node]) {
            throw std::invalid_argument("calculateAllSubtreeLowerBounds: malformed TreeTopology -- "
                                        "internal node leaves no capacity for its subtree");
        }

        const size_t count = std::ceil(static_cast<double>(childrenTotalSize) /
                                       static_cast<double>(capacities[node] - weight));
        const size_t size = childrenTotalSize + count * weight;

        res.emplace(node, TreeLowerBounds(size, count));
    }

    return res;
}

}  // namespace vmp