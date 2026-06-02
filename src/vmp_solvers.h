#ifndef VMP_SOLVERS_H
#define VMP_SOLVERS_H

#include <vmp_clustertree.h>
#include <vmp_maximisers.h>
#include <vmp_packing.h>
#include <vmp_solverutils.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vmp
{

/**
 * Packs `[guestsBegin, guestsEnd)` sequentially by Next Fit, modifying a
 * partial hosts vector
 *
 * @tparam GuestIt any iterator type over `const Guest *`
 * @param capacity the fixed bin capacity
 * @param guestsBegin the start of guest range
 * @param guestsEnd the end of guest range
 * @param hosts the partial hosts vector to use
 */
template <ConstPtrIterator<Guest> GuestIt>
void proceedByNextFit(size_t capacity, GuestIt guestsBegin, GuestIt guestsEnd,
                      std::vector<std::shared_ptr<Host>> &hosts)
{
    for (; guestsBegin != guestsEnd; ++guestsBegin) {
        const auto &guest = *guestsBegin;
        if (hosts.empty() || !hosts.back()->accommodatesGuest(*guest)) {
            hosts.push_back(std::make_shared<Host>(capacity));
        }
        hosts.back()->addGuest(guest);
    }
}

/**
 * Solves an instance of VM-PACK by Next Fit.
 *
 * @param instance the instance to solve
 * @return a valid packing
 */
template <typename InstanceT>
Packing solveByNextFit(const InstanceT &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;

    auto guests = instance.guests();
    proceedByNextFit(instance.capacity(), guests.begin(), guests.end(), hosts);

    return Packing(std::move(hosts));
}

/**
 * Packs `[guestsBegin, guestsEnd)` sequentially by First Fit, modifying a
 * partial hosts vector
 *
 * @tparam GuestIt any iterator type over `const Guest *`
 * @param capacity the fixed bin capacity
 * @param guestsBegin the start of guest range
 * @param guestsEnd the end of guest range
 * @param hosts the partial hosts vector to use
 */
template <ConstPtrIterator<Guest> GuestIt>
void proceedByFirstFit(size_t capacity, GuestIt guestsBegin, GuestIt guestsEnd,
                       std::vector<std::shared_ptr<Host>> &hosts)
{
    for (; guestsBegin != guestsEnd; ++guestsBegin) {
        const auto &guest = *guestsBegin;

        auto hostIter = std::ranges::find_if(
            hosts, [&](const auto &host) { return host->accommodatesGuest(*guest); });

        if (hostIter == hosts.end()) {
            hosts.push_back(std::make_shared<Host>(capacity));
            hostIter = hosts.end() - 1;
        }

        (*hostIter)->addGuest(guest);
    }
}

/**
 * Solves an instance of VM-PACK by First Fit
 *
 * @param instance the instance to solve
 * @return a valid packing
 */
template <typename InstanceT>
Packing solveByFirstFit(const InstanceT &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;

    auto guests = instance.guests();
    proceedByFirstFit(instance.capacity(), guests.begin(), guests.end(), hosts);

    return Packing(std::move(hosts));
}

/**
 * Packs `[guestsBegin, guestsEnd)` sequentially by "Best Fusion" of Grange, et
 * al. (2021), modifying a partial hosts vector
 *
 * @tparam GuestIt any iterator type over `const Guest *`
 * @param capacity the fixed bin capacity
 * @param guestsBegin the start of guest range
 * @param guestsEnd the end of guest range
 * @param hosts the partial hosts vector to use
 */
template <ConstPtrIterator<Guest> GuestIt>
void proceedByEfficiency(size_t capacity, GuestIt guestsBegin, GuestIt guestsEnd,
                         std::vector<std::shared_ptr<Host>> &hosts)
{
    for (; guestsBegin != guestsEnd; ++guestsBegin) {
        const auto &guest = *guestsBegin;

        double bestRelSize = guest->uniquePageCount();
        std::shared_ptr<Host> bestHost = nullptr;

        for (const auto &host : hosts) {
            if (!host->accommodatesGuest(*guest)) {
                continue;
            }

            const double candidateRelSize = calculateRelSize(*guest, host->pageFrequencies());
            if (candidateRelSize <= bestRelSize) {
                bestHost = host;
                bestRelSize = candidateRelSize;
            }
        }

        if (!bestHost) {
            hosts.emplace_back(std::make_shared<Host>(capacity));
            bestHost = hosts.back();
        }
        bestHost->addGuest(*guestsBegin);
    }
}

/**
 * Solves an instance of VM-PACK by "Best Fusion" of Grange, et al. (2021)
 *
 * @param instance the instance to solve
 * @return a valid packing
 */
template <typename InstanceT>
Packing solveByEfficiency(const InstanceT &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;

    auto guests = instance.guests();
    proceedByEfficiency(instance.capacity(), guests.begin(), guests.end(), hosts);

    return Packing(std::move(hosts));
}

/**
 * Packs `[guestsBegin, guestsEnd)` sequentially by "Overload-and-Remove" of
 * Grange, et al. (2021), modifying a partial hosts vector
 *
 * @tparam GuestIt any iterator type over `const Guest *`
 * @param capacity the fixed bin capacity
 * @param guestsBegin the start of guest range
 * @param guestsEnd the end of guest range
 * @param hosts the partial hosts vector to use
 */
template <ConstPtrIterator<Guest> GuestIt>
void proceedByOverloadAndRemove(size_t capacity, GuestIt guestsBegin, GuestIt guestsEnd,
                                std::vector<std::shared_ptr<Host>> &hosts)
{
    std::deque unplaced(guestsBegin, guestsEnd);
    std::unordered_map<const Guest *, std::unordered_set<std::shared_ptr<Host>>>
        attemptedPlacements;

    while (!unplaced.empty()) {
        // Select the best container by relative size
        const auto guest = unplaced.front();
        unplaced.pop_front();

        std::shared_ptr<Host> bestHost = nullptr;
        double bestRelSize = std::numeric_limits<double>::max();

        for (const auto &host : hosts) {
            if (attemptedPlacements[guest].contains(host)) {
                continue;
            }
            const auto candidateRelSize = calculateRelSize(*guest, host->pageFrequencies());
            if (candidateRelSize < bestRelSize) {
                bestHost = host;
                bestRelSize = candidateRelSize;
            }
        }

        if (!bestHost) {
            hosts.emplace_back(std::make_shared<Host>(capacity));
            bestHost = hosts.back();
        }

        bestHost->addGuest(guest);
        attemptedPlacements[guest].insert(bestHost);

        // Remove the worst guest of the container by size-to-relative-size ratio
        while (bestHost->isOverfull()) {
            const auto worstGuest =
                *std::ranges::min_element(bestHost->guests(), {}, [&](const auto &candidate) {
                    return calculateSizeRelRatio(*candidate, bestHost->pageFrequencies());
                });

            unplaced.push_back(worstGuest);
            bestHost->removeGuest(worstGuest);
        }
    }

    // Clear overfull containers and enqueue their guests
    for (const auto &host : hosts) {
        if (!host->isOverfull()) {
            continue;
        }
        for (const auto &guest : host->guests()) {
            unplaced.push_back(guest);
        }
        host->clearGuests();
    }

    proceedByFirstFit(capacity, unplaced.begin(), unplaced.end(), hosts);
}

/**
 * Solves an instance of VM-PACK by "Overload-and-Remove" of Grange, et al. (2021)
 *
 * @param instance the instance to solve
 * @return a valid packing
 */
template <typename InstanceT>
Packing solveByOverloadAndRemove(const InstanceT &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;

    auto guests = instance.guests();
    proceedByOverloadAndRemove(instance.capacity(), guests.begin(), guests.end(), hosts);

    return Packing(std::move(hosts));
}

/**
 * Solves an instance of VM-PACK by method similar to Shao & Liang (2023).
 *
 * @param instance the instance to solve
 * @return a valid packing
 */
template <typename InstanceT>
Packing solveByOpportunityAwareEfficiency(const InstanceT &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;
    auto guests = instance.guests();
    std::unordered_set unplaced(guests.begin(), guests.end());

    while (!unplaced.empty()) {
        const Guest *largestGuest = nullptr;
        const Guest *bestGuest = nullptr;
        std::shared_ptr<Host> bestHost;

        double bestScore = std::numeric_limits<double>::lowest();

        for (const auto &guest : unplaced) {
            if (!largestGuest || guest->uniquePageCount() > largestGuest->uniquePageCount()) {
                largestGuest = guest;
            }

            for (const auto &host : hosts) {
                if (!host->accommodatesGuest(*guest)) {
                    continue;
                }
                const double candidateScore =
                    calculateOpportunityAwareEfficiency(*guest, host, hosts);
                if (candidateScore > bestScore) {
                    bestGuest = guest;
                    bestHost = host;
                    bestScore = candidateScore;
                }
            }
        }

        if (!bestGuest) {
            bestHost = std::make_shared<Host>(instance.capacity());
            bestGuest = largestGuest;
            hosts.push_back(bestHost);
        }
        bestHost->addGuest(bestGuest);
        unplaced.erase(bestGuest);
    }

    return Packing(std::move(hosts));
}

/**
 * Solves VM-PACK by the Sinderal, et al. (2011) greedy algorithm on the tree model.
 *
 * @param instance the instance to solve
 * @param intermediateSolver the intermediate solver with which to pack each extracted subtree
 * @return a valid packing
 */
template <ConstPtrIterator<Guest> GuestIt>
Packing solveByTree(const Tree &tree,
                    void (*intermediateSolver)(size_t, GuestIt, GuestIt,
                                               std::vector<std::shared_ptr<Host>> &))
{
    using NodeId = Tree::NodeId;

    auto workingTree = tree;
    const size_t capacity = tree.capacity();

    std::vector<std::shared_ptr<Host>> hosts;

    while (true) {
        const auto lowerBounds = calculateAllSubtreeLowerBounds(workingTree, capacity);

        if (lowerBounds.at(workingTree.rootNode()).count == 1) {
            const auto &guests = workingTree.guestsOfSubtree(workingTree.rootNode());
            if (guests.empty()) {
                break;
            }

            Host host(capacity);
            host.addGuests(guests.begin(), guests.end());
            assert(!host.isOverfull());

            hosts.push_back(std::make_shared<Host>(std::move(host)));
            break;
        }

        NodeId minNid = std::numeric_limits<NodeId>::max();
        size_t minNodeCount = std::numeric_limits<size_t>::max();

        for (const auto &[nid, bounds] : lowerBounds) {
            if (bounds.count <= 1) {
                continue;
            }

            const auto &children = workingTree.childrenOfNode(nid);

            if (!std::ranges::all_of(children, [&](const NodeId child) {
                    return lowerBounds.at(child).count <= 1;
                })) {
                continue;
            }

            if (minNid == std::numeric_limits<NodeId>::max() || bounds.count < minNodeCount) {
                minNid = nid;
                minNodeCount = bounds.count;
            }
        }

        assert(minNid != std::numeric_limits<NodeId>::max());

        const auto &guestsToPack = workingTree.guestsOfSubtree(minNid);
        intermediateSolver(capacity, guestsToPack.begin(), guestsToPack.end(), hosts);

        if (minNid == workingTree.rootNode()) {
            break;
        }

        workingTree.eraseSubtree(minNid);
    }

    return Packing(std::move(hosts));
}

/**
 * Solves an instance of VM-PACK by searching for the minimum number of bins
 * that yield a complete packing using the given maximisation algorithm.
 *
 * @param instance the instance to solve
 * @param maximiser the n-host maximiser
 * @param allowUnlimitedHosts whether the maximiser will produce a minimal packing when
 * given unlimited allowance
 * @param decantMaximiserOutputs whether to decant the intermediate maximiser outputs
 * @return a packing into minimum maxHosts
 */
template <typename InstanceT>
Packing solveByMaximiser(
    const InstanceT &instance,
    const std::function<Packing(const InstanceT &instance, size_t maxHosts)> &maximiser,
    const bool allowUnlimitedHosts = false, const bool decantMaximiserOutputs = true)
{
    std::optional<Packing> bestPacking;

    if (allowUnlimitedHosts) {
        bestPacking = maximiser(instance, std::numeric_limits<size_t>::max());

        if (decantMaximiserOutputs) {
            bestPacking->decantGuests();
        }
    }
    else {
        // Binary search for the least number of hosts that produces a complete packing
        size_t minHosts = 1;
        size_t maxHosts = instance.guests().size();

        while (minHosts <= maxHosts) {
            const size_t allowedHostCount = minHosts + (maxHosts - minHosts) / 2;
            Packing candidate = maximiser(instance, allowedHostCount);

            if (decantMaximiserOutputs) {
                candidate.decantGuests();
            }

            if (candidate.guestCount() == instance.guests().size()) {
                bestPacking = std::move(candidate);
                maxHosts = allowedHostCount - 1;
            }
            else {
                minHosts = allowedHostCount + 1;
            }
        }
    }

    if (!bestPacking) {
        throw std::runtime_error("no valid packing found -- is a guest larger than the capacity?");
    }

    return std::move(*bestPacking);
}

/**
 * Solves the instance by reduction to the general maximisation problem, then approximate reduction
 * to the one-host maximisation problem, which is approximated by Li, et al. (2009), and in the case
 * where if initialSubsetSize = 1, by Rampersaud & Grosu (2014).
 *
 * @param instance the instance to solve
 * @param initialSubsetSize place guests by computing the efficiency of each possible guest subset
 * of this size
 * @param decantMaximiserOutputs whether to decant the intermediate maximiser outputs
 * @return a valid packing
 */
template <typename InstanceT>
Packing solveByLocalSubsetEfficiency(const InstanceT &instance, const int initialSubsetSize,
                                     const bool decantMaximiserOutputs = true)
{
    auto oneHostMaximiser = [&](const InstanceT &inst,
                                const std::unordered_map<const Guest *, int> &profits) {
        return maximiseOneHostBySubsetEfficiency(inst, profits, initialSubsetSize);
    };

    auto nHostMaximiser = [&](const InstanceT &inst, const size_t maxHosts) {
        return maximiseByLocalSearch<InstanceT>(inst, maxHosts, oneHostMaximiser);
    };

    return solveByMaximiser<InstanceT>(instance, nHostMaximiser, true, decantMaximiserOutputs);
}

/**
 * Solves the instance by reduction to the n-host maximisation problem, then approximate reduction
 * to the one-host maximisation problem, which is approximated by the Sinderal, et al. (2011) DP
 * algorithm on the cluster-tree model
 *
 * @param tree the cluster tree instance to solve
 * @param decantMaximiserOutputs whether to decant the intermediate maximiser outputs
 * @return a valid packing
 */
inline Packing solveByLocalClusterTree(const ClusterTree &tree,
                                       const bool decantMaximiserOutputs = true)
{
    auto oneHostMaximiser = [&](const ClusterTree &tree,
                                const std::unordered_map<const Guest *, int> &profits) {
        return maximiseOneHostByClusterTree(tree, profits);
    };

    auto nHostMaximiser = [&](const ClusterTree &tree, const size_t maxHosts) {
        return maximiseByLocalSearch<ClusterTree>(tree, maxHosts, oneHostMaximiser);
    };

    return solveByMaximiser<ClusterTree>(tree, nHostMaximiser, true, decantMaximiserOutputs);
}

}  // namespace vmp

#endif  // VMP_SOLVERS_H
