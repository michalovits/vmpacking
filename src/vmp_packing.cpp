#include "vmp_solverutils.h"

#include <vmp_packing.h>

namespace vmp
{

Packing::Packing(std::vector<std::shared_ptr<Host>> hosts) : hosts(std::move(hosts)), guestCount(0)
{
    for (const auto &host : this->hosts) {
        guestCount += host->getGuestCount();
    }
}

void Packing::decantGuests()
{
    using SetGuestIt = std::unordered_set<std::shared_ptr<const Guest>>::const_iterator;
    decantGuestByAllPartitioners<SetGuestIt>(hosts);
}

void Packing::addHost(const std::shared_ptr<Host> &host)
{
    // TODO this copy is fine, but Packing is used as an intermediate representation in some
    // solvers. Remove those uses of Packing.
    hosts.push_back(host);
    guestCount += host->getGuestCount();
}

size_t Packing::getGuestCount() const
{
    return guestCount;
}

size_t Packing::getHostCount() const
{
    return hosts.size();
}

std::vector<std::shared_ptr<Host>> &Packing::getHosts()
{
    return hosts;
}

}  // namespace vmp