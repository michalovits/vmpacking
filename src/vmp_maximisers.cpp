#include <vmp_maximisers.h>

#include <vmp_clustertreeinstance.h>

#include <cassert>
#include <queue>
#include <unordered_set>

namespace vmp
{

Host maximiseOneHostBySubsetEfficiency(
    const GeneralInstance &instance,
    const std::unordered_map<std::shared_ptr<const Guest>, int> &profits, int initialSubsetSize)
{
    Host host(instance.getCapacity());
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

    GuestSelection() : pageCount(std::numeric_limits<int>::max()) {}
    GuestSelection(const size_t pageCount, const std::vector<std::shared_ptr<const Guest>> &guests)
        : pageCount(pageCount), guests(guests)
    {
    }

    void setFromSelection(const std::vector<size_t> &selection,
                          const std::unordered_set<int> &pages, const ClusterTreeInstance &instance)
    {
        pageCount = static_cast<int>(pages.size());
        guests.clear();
        guests.reserve(selection.size());

        for (const size_t selected : selection) {
            if (instance.nodeIsLeaf(selected)) {
                guests.push_back(instance.getNodeGuest(selected));
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

    GuestSelection(const GuestSelection &) = default;
    GuestSelection(GuestSelection &&) noexcept = default;
    GuestSelection &operator=(const GuestSelection &) = default;
    GuestSelection &operator=(GuestSelection &&) noexcept = default;
    ~GuestSelection() = default;
};

static std::pair<std::vector<size_t>, std::unordered_set<int>>
selectNodesByMask(const ClusterTreeInstance &instance, const std::vector<size_t> &pool,
                  const uint64_t mask)
{
    std::unordered_set<int> pages;
    std::vector<size_t> selection;

    for (uint64_t i = 0; i < pool.size(); ++i) {
        if (mask & 1ULL << i) {
            const auto &nodePages = instance.getNodePages(pool[i]);
            pages.insert(nodePages.begin(), nodePages.end());
            selection.push_back(pool[i]);
        }
    }

    return { selection, pages };
}

static size_t makeAccessibleChildrenMask(const std::vector<size_t> &children,
                                         const std::vector<size_t> &allowedParents,
                                         const ClusterTreeInstance &instance)
{
    size_t accessibleChildrenMask = 0;
    for (int i = 0; i < children.size(); ++i) {
        const auto &trueParents = instance.getNodeParents(children[i]);

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
                                  const ClusterTreeInstance &instance)
{
    const std::vector<size_t> &nodes = instance.getClusterNodes(cluster);
    assert(nodes.size() < 64);

    std::optional<GuestSelection> lowestCost;

    for (uint64_t selectionMask = 0; selectionMask < 1ULL << nodes.size(); ++selectionMask) {
        if (!checkAllAccessible(selectionMask, accessibleMask)) {
            continue;
        }

        const size_t degree = instance.getClusterChildren(cluster).size();
        // Assume we have already processed the child nodes by topological sort
        const auto &cost = costs.at({ cluster, selectionMask, degree, profitTarget });

        if (!lowestCost.has_value() || cost.pageCount < lowestCost->pageCount) {
            lowestCost = std::move(cost);
        }
    }
    return lowestCost;
}

static std::optional<GuestSelection>
findMostProfitableScenarioAtRoot(const auto &costs, const ClusterTreeInstance &instance)
{
    const size_t root = ClusterTreeInstance::getRootCluster();
    const size_t rootDegree = instance.getClusterChildren(root).size();

    size_t bestProfit = 0;
    std::optional<GuestSelection> bestProfitCost;

    for (auto &[key, value] : costs) {
        if (key.cluster == root && key.childCount == rootDegree && key.profitTarget > bestProfit &&
            value.pageCount <= instance.getCapacity()) {
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

    for (size_t cluster = 0; cluster < instance.getClusterCount(); ++cluster) {
        const size_t childCount = instance.getClusterChildren(cluster).size();

        if ((unvisitedClusterChildCount[cluster] = childCount) == 0) {
            clustersToVisit.push(cluster);
        }
    }

    while (!clustersToVisit.empty()) {
        const size_t cluster = clustersToVisit.front();
        // We consider one cluster at a time, bottom-up
        clustersToVisit.pop();

        // Decrement the unvisited child count of each parent
        const size_t parent = instance.getClusterParent(cluster);
        if (--unvisitedClusterChildCount[parent] == 0) {
            clustersToVisit.push(parent);
        }

        const std::vector<size_t> &curNodes = instance.getClusterNodes(cluster);
        const auto &curChildren = instance.getClusterChildren(cluster);

        if (instance.clusterIsLeaf(cluster)) {
            const auto &guest = instance.getNodeGuest(curNodes.front());
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
                selectNodesByMask(instance, curNodes, curMask);

            size_t profitMade = 0;
            for (const size_t node : curSelection) {
                if (instance.nodeIsLeaf(node)) {
                    profitMade += profits.at(instance.getNodeGuest(node));
                }
            }

            for (size_t profitTarget = 0; profitTarget <= profitUpperBounds[cluster];
                 ++profitTarget) {
                // Initialise:
                // cost[n,s,0,p] = if (sum profit in s) >= p then |union pages in s| else +inf
                ProfitOption curKey{ cluster, curMask, 0, profitTarget };
                if (curSelectionPages.size() > instance.getCapacity() ||
                    profitMade < profitTarget) {
                    costs[curKey] = GuestSelection();
                }
                else {
                    costs[curKey].setFromSelection(curSelection, curSelectionPages, instance);
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
                    const std::vector<size_t> &newChildNodes = instance.getClusterNodes(newChild);
                    const size_t accessibleChildrenMask =
                        makeAccessibleChildrenMask(newChildNodes, curSelection, instance);

                    // Try to make `profitComplement` profit from the newly considered child
                    // cluster
                    for (size_t profitComplement = 0;
                         profitComplement <= std::min(profitTarget, profitUpperBounds.at(newChild));
                         ++profitComplement) {
                        const GuestSelection bestChildCost =
                            findLowestCostAccessibleSelection(
                                costs, newChild, accessibleChildrenMask, profitComplement, instance)
                                .value_or(GuestSelection());
                        const GuestSelection &prevCost =
                            costs.at({ cluster, curMask, j - 1, profitTarget - profitComplement });

                        if (bestChildCost.pageCount == std::numeric_limits<int>::max() ||
                            prevCost.pageCount == std::numeric_limits<int>::max()) {
                            continue;
                        }

                        const size_t candidatePageCount =
                            prevCost.pageCount + bestChildCost.pageCount;

                        if (candidatePageCount <= instance.getCapacity() &&
                            candidatePageCount < costs.at(curKey).pageCount) {
                            costs[curKey].setFromCombinationOfDisjoint(prevCost, bestChildCost);
                        }
                    }
                }
            }
        }
    }

    Host host(instance.getCapacity());

    const auto bestCost = findMostProfitableScenarioAtRoot(costs, instance);
    if (!bestCost.has_value()) {
        return host;
    }

    host.addGuests(bestCost->guests.begin(), bestCost->guests.end());
    return host;
}

}  // namespace vmp