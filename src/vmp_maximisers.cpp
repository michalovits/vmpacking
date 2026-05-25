#include <vmp_maximisers.h>

#include <vmp_clustertreetopology.h>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <queue>
#include <ranges>
#include <unordered_set>

namespace vmp
{

static bool nextComb(std::vector<int> &indices, const int n)
{
    const int k = static_cast<int>(indices.size());
    for (int i = k - 1; i >= 0; --i) {
        if (indices[i] >= n - k + i) {
            continue;
        }
        ++indices[i];
        for (int j = i + 1; j < k; ++j) {
            indices[j] = indices[j - 1] + 1;
        }
        return true;
    }
    return false;
}

/// Find the most efficient subset of guests to place on a host given a
/// mandatory subset size, accounting for the reward and page sharing within
/// the subset and with the host. Return nullopt if impossible.
static std::optional<std::vector<std::pair<std::shared_ptr<const Guest>, int>>>
findMostEfficientSubset(const std::unordered_map<std::shared_ptr<const Guest>, int> &unplaced,
                        const Host &host, int subsetSize)
{
    std::vector<std::pair<std::shared_ptr<const Guest>, int>> guests(unplaced.begin(),
                                                                     unplaced.end());
    const int guestCount = static_cast<int>(guests.size());
    subsetSize = std::min(guestCount, subsetSize);

    std::optional<std::vector<std::pair<std::shared_ptr<const Guest>, int>>> bestSubset;
    double bestSubsetValue = 0.0;

    std::vector<int> indices(subsetSize);
    std::iota(indices.begin(), indices.end(), 0);

    do {
        std::vector<std::pair<std::shared_ptr<const Guest>, int>> subset;
        subset.reserve(subsetSize);
        for (const int index : indices) {
            subset.emplace_back(guests[index]);
        }

        std::vector<std::shared_ptr<const Guest>> candidateView;
        candidateView.reserve(subsetSize);
        for (const auto &guest : subset | std::views::keys) {
            candidateView.push_back(guest);
        }

        if (!host.accommodatesGuests(candidateView.begin(), candidateView.end())) {
            continue;
        }

        const double rewardSum =
            std::accumulate(subset.begin(), subset.end(), 0.0,
                            [](double acc, const auto &guest) { return acc + guest.second; });

        const size_t pageCount =
            host.countPagesWithGuests(candidateView.begin(), candidateView.end());
        const double subsetValue = rewardSum / static_cast<double>(1 + pageCount);

        if (subsetValue > bestSubsetValue) {
            bestSubset = std::move(subset);
            bestSubsetValue = subsetValue;
        }
    } while (nextComb(indices, guestCount));

    return bestSubset;
}

Host maximiseOneHostBySubsetEfficiency(
    const GeneralInstance &instance,
    const std::unordered_map<std::shared_ptr<const Guest>, int> &profits, int initialSubsetSize)
{
    Host host(instance.capacity());
    std::unordered_map<std::shared_ptr<const Guest>, int> unplaced = profits;

    while (!unplaced.empty()) {
        auto bestGuestSet = findMostEfficientSubset(unplaced, host, initialSubsetSize);
        // Try to reduce the subset size until we find a subset that can be accommodated
        while (!bestGuestSet.has_value() && --initialSubsetSize > 0) {
            bestGuestSet = findMostEfficientSubset(unplaced, host, initialSubsetSize);
        }

        if (!bestGuestSet.has_value()) {
            break;
        }

        for (const auto &guest : bestGuestSet.value() | std::views::keys) {
            unplaced.erase(guest);
            host.addGuest(guest);
        }
    }

    return host;
}

/* Suppose s is a subset of the nodes of a cluster n, and p is a profit
 * target. cost[n, s, j, p] is the least page count by which we can achieve
 * >= p by packing s on the server and optionally using nodes from the
 * first j children clusters of n.*/
struct ProfitOption
{
    size_t cluster;        // scenario considers the subtree rooted here
    size_t selectionMask;  // the nodes selected from this cluster
    size_t childCount;  // the subtrees rooted at children [0...childCount-1] of this cluster can be
                        // used
    size_t profitTarget;

    bool operator==(const ProfitOption &) const = default;

    [[nodiscard]] size_t hash() const
    {
        size_t h = std::hash<size_t>{}(cluster);
        auto mix = [&h](const size_t v) {
            h ^= std::hash<size_t>{}(v) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        };
        mix(selectionMask);
        mix(childCount);
        mix(profitTarget);
        return h;
    }
};

struct GuestSelection
{
    size_t pageCount;

    // TODO avoid copying guests, instead reference other entries in the cost table and backtrack
    std::vector<std::shared_ptr<const Guest>> guests;

    GuestSelection() : pageCount(std::numeric_limits<size_t>::max()) {}
    GuestSelection(const size_t pageCount, std::vector<std::shared_ptr<const Guest>> guests)
        : pageCount(pageCount), guests(std::move(guests))
    {
    }

    void setFromSelection(const std::vector<size_t> &selection,
                          const std::unordered_set<int> &pages, const ClusterTreeTopology &tree)
    {
        pageCount = pages.size();
        guests.clear();
        guests.reserve(selection.size());

        for (const size_t selected : selection) {
            if (tree.isLeafNode(selected)) {
                guests.push_back(tree.guestOfNode(selected));
            }
        }
    }

    void setFromCombinationOfDisjoint(const GuestSelection &a, const GuestSelection &b)
    {
        pageCount = a.pageCount + b.pageCount;

        guests.clear();
        guests.reserve(a.guests.size() + b.guests.size());
        guests.insert(guests.end(), a.guests.begin(), a.guests.end());
        guests.insert(guests.end(), b.guests.begin(), b.guests.end());
    }
};

static std::pair<std::vector<size_t>, std::unordered_set<int>>
selectNodesByMask(const ClusterTreeTopology &tree, const std::vector<size_t> &pool,
                  const uint64_t mask)
{
    std::unordered_set<int> pages;
    std::vector<size_t> selection;

    for (uint64_t i = 0; i < pool.size(); ++i) {
        if (mask & 1ULL << i) {
            const auto &nodePages = tree.pagesOfNode(pool[i]);
            pages.insert(nodePages.begin(), nodePages.end());
            selection.push_back(pool[i]);
        }
    }

    return { selection, pages };
}

static size_t makeAccessibleChildrenMask(const std::vector<size_t> &children,
                                         const std::vector<size_t> &allowedParents,
                                         const ClusterTreeTopology &tree)
{
    size_t accessibleChildrenMask = 0;
    for (size_t i = 0; i < children.size(); ++i) {
        const auto &trueParents = tree.parentsOfNode(children[i]);

        for (const size_t parentCandidate : allowedParents) {
            if (std::ranges::find(trueParents, parentCandidate) != trueParents.end()) {
                accessibleChildrenMask |= 1ULL << i;
                break;
            }
        }
    }
    return accessibleChildrenMask;
}

static bool checkAllAccessible(const size_t childrenMask, const size_t accessibleMask)
{
    return (childrenMask & ~accessibleMask) == 0;
}

static std::optional<GuestSelection>
findLowestCostAccessibleSelection(const auto &costs, const size_t cluster,
                                  const size_t accessibleMask, const size_t profitTarget,
                                  const ClusterTreeTopology &tree)
{
    const std::vector<size_t> &nodes = tree.nodesOfCluster(cluster);
    assert(nodes.size() < 64);

    std::optional<GuestSelection> lowestCost;

    for (uint64_t selectionMask = 0; selectionMask < 1ULL << nodes.size(); ++selectionMask) {
        if (!checkAllAccessible(selectionMask, accessibleMask)) {
            continue;
        }

        const size_t degree = tree.childrenOfCluster(cluster).size();
        // Assume we have already processed the child nodes by topological sort
        const auto &cost = costs.at({ cluster, selectionMask, degree, profitTarget });

        if (!lowestCost.has_value() || cost.pageCount < lowestCost->pageCount) {
            lowestCost = std::move(cost);
        }
    }
    return lowestCost;
}

static std::optional<GuestSelection>
findMostProfitableScenarioAtRoot(const auto &costs, const ClusterTreeTopology &tree,
                                 const size_t capacity)
{
    const size_t root = tree.rootCluster();
    const size_t rootDegree = tree.childrenOfCluster(root).size();

    size_t bestProfit = 0;
    std::optional<GuestSelection> bestProfitCost;

    for (auto &[key, value] : costs) {
        if (key.cluster == root && key.childCount == rootDegree && key.profitTarget > bestProfit &&
            value.pageCount <= capacity) {
            bestProfit = key.profitTarget;
            bestProfitCost = std::move(value);
        }
    }

    return bestProfitCost;
}

Host maximiseOneHostByClusterTree(
    const ClusterTreeInstance &instance,
    const std::unordered_map<std::shared_ptr<const Guest>, int> &profits)
{
    const auto &tree = instance.topology();
    const size_t capacity = instance.capacity();

    std::unordered_map<ProfitOption, GuestSelection,
                       decltype([](const ProfitOption &k) { return k.hash(); })>
        costs;

    // The profit upper bound at each subtree is the sum of the profits in its leaves
    std::unordered_map<size_t, size_t> profitUpperBounds;

    // Topological sort:
    // Track the number of unvisited children for each cluster
    // We add a cluster to the frontier only once it has 0 unvisited children
    // As we need the cost table to have been computer for all its child entries
    std::unordered_map<size_t, size_t> unvisitedClusterChildCount;
    std::queue<size_t> clustersToVisit;

    for (size_t cluster = 0; cluster < tree.clusterCount(); ++cluster) {
        const size_t childCount = tree.childrenOfCluster(cluster).size();

        if ((unvisitedClusterChildCount[cluster] = childCount) == 0) {
            clustersToVisit.push(cluster);
        }
    }

    while (!clustersToVisit.empty()) {
        const size_t cluster = clustersToVisit.front();
        // We consider one cluster at a time, bottom-up
        clustersToVisit.pop();

        // Decrement the unvisited child count of each parent
        const size_t parent = tree.parentOfCluster(cluster);
        if (--unvisitedClusterChildCount[parent] == 0) {
            clustersToVisit.push(parent);
        }

        const std::vector<size_t> &curNodes = tree.nodesOfCluster(cluster);
        const auto &curChildren = tree.childrenOfCluster(cluster);

        if (tree.isLeafCluster(cluster)) {
            const auto &guest = tree.guestOfNode(curNodes.front());
            profitUpperBounds[cluster] = profits.at(guest);
        }
        else {
            for (const size_t child : curChildren) {
                profitUpperBounds[cluster] += profitUpperBounds.at(child);
            }
        }

        assert(curNodes.size() < 64);
        // Begin by considering every one of 2^(node count) choices of nodes from this cluster
        for (uint64_t curMask = 0; curMask < 1ULL << curNodes.size(); ++curMask) {
            const auto [curSelection, curSelectionPages] =
                selectNodesByMask(tree, curNodes, curMask);

            size_t profitMade = 0;
            for (const size_t node : curSelection) {
                if (tree.isLeafNode(node)) {
                    profitMade += profits.at(tree.guestOfNode(node));
                }
            }

            for (size_t profitTarget = 0; profitTarget <= profitUpperBounds[cluster];
                 ++profitTarget) {
                // Initialise:
                // cost[n,s,0,p] = if (sum profit in s) >= p then |union pages in s| else +inf
                ProfitOption curKey{ cluster, curMask, 0, profitTarget };
                if (curSelectionPages.size() > capacity || profitMade < profitTarget) {
                    costs[curKey] = GuestSelection();
                }
                else {
                    costs[curKey].setFromSelection(curSelection, curSelectionPages, tree);
                }

                // Allow taking from the first j children at a time
                for (size_t j = 1; j <= curChildren.size(); ++j) {
                    // We will compute cost[n, s, j, p]
                    curKey = { cluster, curMask, j, profitTarget };
                    // Try to do better than with j - 1 children
                    costs[curKey] = costs.at({ cluster, curMask, j - 1, profitTarget });

                    // Only those nodes in the child cluster that have at least one parent in
                    // the current selection are accessible, as the mask must be over all the
                    // cluster's nodes

                    // TODO as a performance optimisation, consider computing the subset of viable
                    // children first, then generate *its* subsets instead

                    const size_t newChild = curChildren[j - 1];
                    const std::vector<size_t> &newChildNodes = tree.nodesOfCluster(newChild);
                    const size_t accessibleChildrenMask =
                        makeAccessibleChildrenMask(newChildNodes, curSelection, tree);

                    // Try to make `profitComplement` profit from the newly considered child
                    // cluster
                    for (size_t profitComplement = 0;
                         profitComplement <= std::min(profitTarget, profitUpperBounds.at(newChild));
                         ++profitComplement) {
                        const auto bestChildCost =
                            findLowestCostAccessibleSelection(
                                costs, newChild, accessibleChildrenMask, profitComplement, tree)
                                .value_or(GuestSelection());
                        const auto &prevCost =
                            costs.at({ cluster, curMask, j - 1, profitTarget - profitComplement });

                        if (bestChildCost.pageCount == std::numeric_limits<size_t>::max() ||
                            prevCost.pageCount == std::numeric_limits<size_t>::max()) {
                            continue;
                        }

                        const size_t candidatePageCount =
                            prevCost.pageCount + bestChildCost.pageCount;

                        if (candidatePageCount <= capacity &&
                            candidatePageCount < costs.at(curKey).pageCount) {
                            costs[curKey].setFromCombinationOfDisjoint(prevCost, bestChildCost);
                        }
                    }
                }
            }
        }
    }

    Host host(capacity);

    const auto bestCost = findMostProfitableScenarioAtRoot(costs, tree, capacity);
    if (!bestCost.has_value()) {
        return host;
    }

    host.addGuests(bestCost->guests.begin(), bestCost->guests.end());
    return host;
}

}  // namespace vmp