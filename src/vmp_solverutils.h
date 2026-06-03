#ifndef VMP_SOLVERUTILS_H
#define VMP_SOLVERUTILS_H

#include <algorithm>
#include <iterator>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <vmp_commontypes.h>
#include <vmp_guest.h>
#include <vmp_host.h>
#include <vmp_tree.h>

namespace vmp
{

double calculateRelSize(const Guest &guest, const std::unordered_map<int, int> &pageFreq);

double calculateSizeRelRatio(const Guest &guest, const std::unordered_map<int, int> &pageFreq);

double calculateOpportunityAwareEfficiency(const Guest &guest, const Host &host,
                                           const std::vector<std::unique_ptr<Host>> &hosts);

template <ConstPtrIterator<Guest> GuestIt>
void decantGuests(std::vector<std::unique_ptr<Host>> &hosts,
                  std::vector<std::vector<const Guest *>> (*partitionGuests)(GuestIt, GuestIt))
{
    for (auto leftIt = hosts.begin(); leftIt != hosts.end(); ++leftIt) {
        const auto &leftHost = *leftIt;

        for (auto rightIt = leftIt + 1; rightIt != hosts.end(); ++rightIt) {
            const auto &rightHost = *rightIt;
            const auto &rightGuests = rightHost->guests();

            const auto partitions = partitionGuests(rightGuests.begin(), rightGuests.end());

            for (const auto &partition : partitions) {
                if (!leftHost->accommodatesGuests(partition.begin(), partition.end())) {
                    continue;
                }

                for (const auto &guest : partition) {
                    leftHost->addGuest(guest);
                    rightHost->removeGuest(guest);
                }
            }
        }
    }
    std::erase_if(hosts, [](const auto &host) { return host->guests().empty(); });
}

template <ConstPtrIterator<Guest> GuestIt>
std::unordered_map<int, int> calculatePageFrequencies(GuestIt guestsBegin, GuestIt guestsEnd)
{
    std::unordered_map<int, int> frequencies;
    for (; guestsBegin != guestsEnd; ++guestsBegin) {
        for (const auto &page : (*guestsBegin)->pages) {
            ++frequencies[page];
        }
    }
    return frequencies;
}

template <UniquePtrIterator<Host> HostIt>
std::unordered_map<int, int> calculatePageFrequencies(HostIt hostsBegin, HostIt hostsEnd)
{
    std::unordered_map<int, int> frequencies;
    for (; hostsBegin != hostsEnd; ++hostsBegin) {
        for (const auto &guest : (*hostsBegin)->guests()) {
            for (const auto &page : guest->pages) {
                ++frequencies[page];
            }
        }
    }
    return frequencies;
}

template <ConstPtrIterator<Guest> GuestIt>
std::vector<std::vector<const Guest *>> partitionAllGuestsTogether(GuestIt guestsBegin,
                                                                   GuestIt guestsEnd)
{
    // Whole-page decanting
    std::vector<const Guest *> allGuests;
    for (; guestsBegin != guestsEnd; ++guestsBegin) {
        allGuests.push_back(*guestsBegin);
    }
    return { allGuests };
}

template <ConstPtrIterator<Guest> GuestIt>
std::vector<std::vector<const Guest *>> partitionGuestsIndividually(GuestIt guestsBegin,
                                                                    GuestIt guestsEnd)
{
    // Per-guest decanting
    std::vector<std::vector<const Guest *>> partitions;
    for (; guestsBegin != guestsEnd; ++guestsBegin) {
        partitions.push_back(std::vector{ *guestsBegin });
    }
    return partitions;
}

inline bool guestsHaveSharedPage(const Guest &guest1, const Guest &guest2)
{
    const auto &smaller = guest1.pages.size() < guest2.pages.size() ? guest1.pages : guest2.pages;
    const auto &larger = guest1.pages.size() < guest2.pages.size() ? guest2.pages : guest1.pages;

    return std::ranges::any_of(smaller.begin(), smaller.end(),
                               [&](const auto &page) { return larger.contains(page); });
}

template <ConstPtrIterator<Guest> GuestIt>
std::vector<std::vector<const Guest *>> partitionConnectedGuestsTogether(GuestIt guestsBegin,
                                                                         GuestIt guestsEnd)
{
    std::vector<std::vector<const Guest *>> result;
    std::unordered_set<const Guest *> discovered;

    for (; guestsBegin != guestsEnd; ++guestsBegin) {
        if (discovered.contains(*guestsBegin)) {
            continue;
        }

        std::vector<const Guest *> component;
        std::queue<const Guest *> frontier;
        frontier.push(*guestsBegin);

        while (!frontier.empty()) {
            const auto guest = frontier.front();
            frontier.pop();

            component.push_back(guest);

            for (auto it = std::next(guestsBegin); it != guestsEnd; ++it) {
                const auto childCandidate = *it;

                if (!discovered.contains(childCandidate) &&
                    guestsHaveSharedPage(*childCandidate, *guest)) {
                    frontier.push(childCandidate);
                    discovered.insert(childCandidate);
                }
            }
        }

        result.push_back(std::move(component));
    }

    return result;
}

template <ConstPtrIterator<Guest> GuestIt>
void decantGuestByAllPartitioners(std::vector<std::unique_ptr<Host>> &hosts)
{
    decantGuests<GuestIt>(hosts, partitionAllGuestsTogether<GuestIt>);
    decantGuests<GuestIt>(hosts, partitionConnectedGuestsTogether<GuestIt>);
    decantGuests<GuestIt>(hosts, partitionGuestsIndividually<GuestIt>);
}

struct TreeLowerBounds
{
    size_t size;   // The total number of pages to pack a subtree
    size_t count;  // The number of hosts required to pack the subtree

    TreeLowerBounds(const size_t size, const size_t count) : size(size), count(count) {}
};

std::unordered_map<Tree::NodeId, TreeLowerBounds>
calculateAllSubtreeLowerBounds(const Tree &tree, const size_t capacity);

bool nextComb(std::vector<int> &indices, const int n);

}  // namespace vmp

#endif  // VMP_SOLVERUTILS_H
