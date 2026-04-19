#ifndef VMP_MAXIMISERS_H
#define VMP_MAXIMISERS_H

#include <vmp_clustertreeinstance.h>
#include <vmp_commontypes.h>
#include <vmp_packing.h>

#include <numeric>
#include <ranges>

namespace vmp
{

static bool next_combination(std::vector<int> &indices, const size_t n)
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

/**
 * Finds the most efficient subset of guests to place on a host given a
 * mandatory subset size, accounting for the reward and page sharing within
 * the subset and with the host.
 *
 * @param unplaced the pool of guests to sample
 * @param host the host to place the guests on
 * @param subsetSize the number of guests to place
 * @return the most efficient subset of guests, or `std::nullopt` if no viable subset exists
 */
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
    } while (next_combination(indices, guestCount));

    return bestSubset;
}

/**
 * Places guests on a single host by always picking the next most valuable set
 * based on the reward and page sharing. See Li, et al. (2009) and Rampersaud &
 * Grosu (2014), who proposed a similar algorithm with initialSubsetSize = 1.
 *
 * @param instance the instance to maximise
 * @param profits the profit acquired by packing each guest
 * @param initialSubsetSize the initial subset size to try. Defaults to 1.
 * @return a host with the most valuable guests placed
 */
Host maximiseOneHostBySubsetEfficiency(
    const GeneralInstance &instance,
    const std::unordered_map<std::shared_ptr<const Guest>, int> &profits,
    int initialSubsetSize = 1);

/**
 * Maximises the number of guests placed on a single host on the Cluster Tree
 * model. See Sinderal, et al. (2011).
 *
 * @param instance the instance to maximise
 * @param profits the profit acquired by packing each guest
 * @return the maximised host
 */
Host maximiseOneHostByClusterTree(
    const ClusterTreeInstance &instance,
    const std::unordered_map<std::shared_ptr<const Guest>, int> &profits);

/**
 * Maximises the number of guests placed on `allowedHostCount` hosts by using a
 * single-host maximiser. Inspired by Fleischer, et al. (2006).
 *
 * @param instance the instance to maximise
 * @param allowedHostCount the number of hosts to use
 * @param oneHostMaximiser the single-host maximiser to use
 * @return a packing with at most `allowedHostCount` hosts
 */
template <typename InstanceType>
    requires Instance<InstanceType>
Packing maximiseByLocalSearch(
    const InstanceType &instance, const size_t allowedHostCount,
    const std::function<Host(const InstanceType &,
                             const std::unordered_map<std::shared_ptr<const Guest>, int> &)>
        &oneHostMaximiser)
{
    std::vector<std::shared_ptr<Host>> hosts;
    std::unordered_map<std::shared_ptr<const Guest>, int> profits;

    for (const auto &guest : instance.getGuests()) {
        profits[guest] = 1;
    }

    size_t placed = 0;
    while (placed < instance.getGuests().size() && hosts.size() < allowedHostCount) {
        Host newHost = oneHostMaximiser(instance, profits);

        // Profits are monotonically non-increasing; once the maximiser returns
        // an empty host, no further iteration can make progress
        if (newHost.getGuests().empty()) {
            break;
        }

        for (const auto &guest : newHost.getGuests()) {
            profits[guest] = 0;
        }

        placed += newHost.getGuests().size();
        hosts.emplace_back(std::make_shared<Host>(std::move(newHost)));
    }

    return Packing(hosts);
}

}  // namespace vmp

#endif  // VMP_MAXIMISERS_H
