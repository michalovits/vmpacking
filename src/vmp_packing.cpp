#include "vmp_solverutils.h"

#include <vmp_packing.h>

namespace vmp
{

Packing::Packing(std::vector<std::shared_ptr<Host>> hosts)
    : hosts_(std::move(hosts)), guestCount_(0)
{
    for (const auto &host : this->hosts_) {
        guestCount_ += host->guestCount();
    }
}

void Packing::decantGuests()
{
    using SetGuestIt = std::unordered_set<const Guest *>::const_iterator;
    decantGuestByAllPartitioners<SetGuestIt>(hosts_);
}

void Packing::addHost(const std::shared_ptr<Host> &host)
{
    // TODO this copy is fine, but Packing is used as an intermediate representation in some
    // solvers. Remove those uses of Packing.
    hosts_.push_back(host);
    guestCount_ += host->guestCount();
}

size_t Packing::guestCount() const
{
    return guestCount_;
}

size_t Packing::hostCount() const
{
    return hosts_.size();
}

std::vector<std::shared_ptr<Host>> &Packing::hosts()
{
    return hosts_;
}

}  // namespace vmp