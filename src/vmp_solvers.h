#ifndef VMP_SOLVERS_H
#define VMP_SOLVERS_H

#include <cassert>
#include <iostream>
#include <vmp_packing.h>
#include <vmp_solverutils.h>

namespace vmp
{

/**
 * Packs `[guestsBegin, guestsEnd)` sequentially by Next Fit, modifying a
 * partial hosts vector
 *
 * @tparam GuestIt any iterator type over `std::shared_ptr<const Guest>`
 * @param capacity the fixed bin capacity
 * @param guestsBegin the start of guest range
 * @param guestsEnd the end of guest range
 * @param hosts the partial hosts vector to use
 */
template <SharedPtrIterator<const Guest> GuestIt>
static void proceedByNextFit(size_t capacity, GuestIt guestsBegin, GuestIt guestsEnd,
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
template <typename InstanceType>
Packing solveByNextFit(const InstanceType &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;

    auto guests = instance.getGuests();
    proceedByNextFit(instance.getCapacity(), guests.begin(), guests.end(), hosts);

    return Packing(hosts);
}

/**
 * Packs `[guestsBegin, guestsEnd)` sequentially by First Fit, modifying a
 * partial hosts vector
 *
 * @tparam GuestIt any iterator type over `std::shared_ptr<const Guest>`
 * @param capacity the fixed bin capacity
 * @param guestsBegin the start of guest range
 * @param guestsEnd the end of guest range
 * @param hosts the partial hosts vector to use
 */
template <SharedPtrIterator<const Guest> GuestIt>
static void proceedByFirstFit(size_t capacity, GuestIt guestsBegin, GuestIt guestsEnd,
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
template <typename InstanceType>
Packing solveByFirstFit(const InstanceType &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;

    const auto &guests = instance.getGuests();
    proceedByFirstFit(instance.getCapacity(), guests.begin(), guests.end(), hosts);

    return Packing(hosts);
}

/**
 * Packs `[guestsBegin, guestsEnd)` sequentially by "Best Fusion" of Grange, et
 * al. (2021), modifying a partial hosts vector
 *
 * @tparam GuestIt any iterator type over `std::shared_ptr<const Guest>`
 * @param capacity the fixed bin capacity
 * @param guestsBegin the start of guest range
 * @param guestsEnd the end of guest range
 * @param hosts the partial hosts vector to use
 */
template <SharedPtrIterator<const Guest> GuestIt>
static void proceedByEfficiency(size_t capacity, GuestIt guestsBegin, GuestIt guestsEnd,
                                std::vector<std::shared_ptr<Host>> &hosts)
{
    for (; guestsBegin != guestsEnd; ++guestsBegin) {
        const auto &guest = *guestsBegin;

        double bestRelSize = guest->getUniquePageCount();
        std::shared_ptr<Host> bestHost = nullptr;

        for (const auto &host : hosts) {
            if (!host->accommodatesGuest(*guest)) {
                continue;
            }

            const double candidateRelSize = calculateRelSize(*guest, host->getPageFrequencies());
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
template <typename InstanceType>
Packing solveByEfficiency(const InstanceType &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;

    const auto &guests = instance.getGuests();
    proceedByEfficiency(instance.getCapacity(), guests.begin(), guests.end(), hosts);

    return Packing(hosts);
}

/**
 * Packs `[guestsBegin, guestsEnd)` sequentially by "Overload-and-Remove" of
 * Grange, et al. (2021), modifying a partial hosts vector
 *
 * @tparam GuestIt any iterator type over `std::shared_ptr<const Guest>`
 * @param capacity the fixed bin capacity
 * @param guestsBegin the start of guest range
 * @param guestsEnd the end of guest range
 * @param hosts the partial hosts vector to use
 */
template <SharedPtrIterator<const Guest> GuestIt>
static void proceedByOverloadAndRemove(size_t capacity, GuestIt guestsBegin, GuestIt guestsEnd,
                                       std::vector<std::shared_ptr<Host>> &hosts)
{
    std::deque unplaced(guestsBegin, guestsEnd);
    std::unordered_map<std::shared_ptr<const Guest>, std::unordered_set<std::shared_ptr<Host>>>
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
            const auto candidateRelSize = calculateRelSize(*guest, host->getPageFrequencies());
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
                *std::ranges::min_element(bestHost->getGuests(), {}, [&](const auto &candidate) {
                    return calculateSizeRelRatio(*candidate, bestHost->getPageFrequencies());
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
        for (const auto &guest : host->getGuests()) {
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
template <typename InstanceType>
Packing solveByOverloadAndRemove(const InstanceType &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;

    const auto &guests = instance.getGuests();
    proceedByOverloadAndRemove(instance.getCapacity(), guests.begin(), guests.end(), hosts);

    return Packing(hosts);
}

/**
 * Solves an instance of VM-PACK by method similar to Shao & Liang (2023).
 *
 * @param instance the instance to solve
 * @return a valid packing
 */
template <typename InstanceType>
Packing solveByOpportunityAwareEfficiency(const InstanceType &instance)
{
    std::vector<std::shared_ptr<Host>> hosts;
    const auto &guests = instance.getGuests();
    std::unordered_set unplaced(guests.begin(), guests.end());

    while (!unplaced.empty()) {
        std::shared_ptr<const Guest> largestGuest;
        std::shared_ptr<const Guest> bestGuest;
        std::shared_ptr<Host> bestHost;

        double bestScore = std::numeric_limits<double>::min();

        for (const auto &guest : unplaced) {
            if (!largestGuest || guest->getUniquePageCount() > largestGuest->getUniquePageCount()) {
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
            bestHost = std::make_shared<Host>(instance.getCapacity());
            bestGuest = largestGuest;
            hosts.push_back(bestHost);
        }
        bestHost->addGuest(bestGuest);
        unplaced.erase(bestGuest);
    }

    return Packing(hosts);
}

/**
 * Solves VM-PACK by the Sinderal, et al. (2011) greedy algorithm on the tree model.
 *
 * @param instance the instance to solve
 * @param intermediateSolver the intermediate solver with which to pack each extracted subtree
 * @return a valid packing
 */
template <SharedPtrIterator<const Guest> GuestIt>
Packing solveByTree(const TreeInstance &instance,
                    void (*intermediateSolver)(size_t, GuestIt, GuestIt,
                                               std::vector<std::shared_ptr<Host>> &))
{
    TreeInstance workingInstance = instance;

    std::vector<std::shared_ptr<Host>> hosts;

    while (true) {
        const auto lowerBounds = calculateAllSubtreeLowerBounds(workingInstance);

        if (lowerBounds.at(TreeInstance::getRootNode()).count == 1) {
            const auto &guests = workingInstance.getGuests();
            if (guests.empty()) {
                break;
            }

            Host host(workingInstance.getCapacity());
            host.addGuests(guests.begin(), guests.end());

            hosts.push_back(std::make_shared<Host>(std::move(host)));
            break;
        }

        size_t minNode = std::numeric_limits<size_t>::max();
        size_t minNodeCount = std::numeric_limits<size_t>::max();

        for (const auto &[node, bounds] : lowerBounds) {
            if (bounds.count <= 1) {
                continue;
            }

            const auto &children = workingInstance.getNodeChildren(node);

            if (!std::ranges::all_of(children, [&](const size_t child) {
                    return lowerBounds.at(child).count <= 1;
                })) {
                continue;
            }

            if (minNode == std::numeric_limits<size_t>::max() || bounds.count < minNodeCount) {
                minNode = node;
                minNodeCount = bounds.count;
            }
        }

        assert(minNode != std::numeric_limits<size_t>::max());

        const auto &guestsToPack = workingInstance.getSubtreeGuests(minNode);
        intermediateSolver(instance.getCapacity(), guestsToPack.begin(), guestsToPack.end(), hosts);

        if (minNode == TreeInstance::getRootNode()) {
            break;
        }

        workingInstance.removeSubtree(minNode);
    }

    return Packing(hosts);
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
template <typename InstanceType>
Packing solveByLocalSubsetEfficiency(const InstanceType &instance, const int initialSubsetSize,
                                     const bool decantMaximiserOutputs = true)
{
    auto oneHostMaximiser =
        [&](const InstanceType &inst,
            const std::unordered_map<std::shared_ptr<const Guest>, int> &profits) {
            return maximiseOneHostBySubsetEfficiency(inst, profits, initialSubsetSize);
        };

    auto nHostMaximiser = [&](const InstanceType &inst, const size_t maxHosts) {
        return maximiseByLocalSearch<InstanceType>(inst, maxHosts, oneHostMaximiser);
    };

    return solveByMaximiser<InstanceType>(instance, nHostMaximiser, true, decantMaximiserOutputs);
}

/**
 * Solves the instance by reduction to the n-host maximisation problem, then approximate reduction
 * to the one-host maximisation problem, which is approximated by the Sinderal, et al. (2011) DP
 * algorithm on the cluster-tree model
 *
 * @param instance the instance to solve
 * @param decantMaximiserOutputs whether to decant the intermediate maximiser outputs
 * @return a valid packing
 */
template <typename ClusterTreeInstance>
Packing solveByLocalClusterTree(const ClusterTreeInstance &instance,
                                const bool decantMaximiserOutputs = true)
{
    auto oneHostMaximiser =
        [&](const ClusterTreeInstance &inst,
            const std::unordered_map<std::shared_ptr<const Guest>, int> &profits) {
            return maximiseOneHostByClusterTree(inst, profits);
        };

    auto nHostMaximiser = [&](const ClusterTreeInstance &inst, const size_t maxHosts) {
        return maximiseByLocalSearch<ClusterTreeInstance>(inst, maxHosts, oneHostMaximiser);
    };

    return solveByMaximiser<ClusterTreeInstance>(instance, nHostMaximiser, true,
                                                 decantMaximiserOutputs);
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
template <typename InstanceType>
    requires Instance<InstanceType>
Packing solveByMaximiser(
    const InstanceType &instance,
    const std::function<Packing(const InstanceType &instance, size_t maxHosts)> &maximiser,
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
        size_t maxHosts = instance.getGuests().size();

        while (minHosts <= maxHosts) {
            const size_t allowedHostCount = minHosts + (maxHosts - minHosts) / 2;
            Packing candidate = maximiser(instance, allowedHostCount);

            if (decantMaximiserOutputs) {
                candidate.decantGuests();
            }

            if (candidate.getGuestCount() == instance.getGuests().size()) {
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

    return Packing(bestPacking->getHosts());
}

}  // namespace vmp

#endif  // VMP_SOLVERS_H
