#ifndef VMP_MAXIMISERS_H
#define VMP_MAXIMISERS_H

#include <vmp_clustertreetopology.h>
#include <vmp_generaltopology.h>
#include <vmp_packing.h>

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vmp
{

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
template <typename InstanceT>
Packing maximiseByLocalSearch(
    const InstanceT &instance, const size_t allowedHostCount,
    const std::function<Host(const InstanceT &,
                             const std::unordered_map<std::shared_ptr<const Guest>, int> &)>
        &oneHostMaximiser)
{
    std::vector<std::shared_ptr<Host>> hosts;
    std::unordered_map<std::shared_ptr<const Guest>, int> profits;

    for (const auto &guest : instance.guests()) {
        profits[guest] = 1;
    }

    size_t placed = 0;
    while (placed < instance.guests().size() && hosts.size() < allowedHostCount) {
        Host newHost = oneHostMaximiser(instance, profits);

        // Profits are monotonically non-increasing; once the maximiser returns
        // an empty host, no further iteration can make progress
        if (newHost.guests().empty()) {
            break;
        }

        for (const auto &guest : newHost.guests()) {
            profits[guest] = 0;
        }

        placed += newHost.guests().size();
        hosts.emplace_back(std::make_shared<Host>(std::move(newHost)));
    }

    return Packing(std::move(hosts));
}

}  // namespace vmp

#endif  // VMP_MAXIMISERS_H
