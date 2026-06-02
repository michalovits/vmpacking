#include "vmp_solverutils.h"

#include <vmp_packing.h>

namespace vmp
{

Packing::Packing(std::vector<std::unique_ptr<Host>> hosts)
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

void Packing::addHost(std::unique_ptr<Host> host)
{
    // TODO this copy is fine, but Packing is used as an intermediate representation in some
    // solvers. Remove those uses of Packing.
    guestCount_ += host->guestCount();
    hosts_.push_back(std::move(host));
}

size_t Packing::guestCount() const
{
    return guestCount_;
}

size_t Packing::hostCount() const
{
    return hosts_.size();
}

std::vector<std::unique_ptr<Host>> &Packing::hosts()
{
    return hosts_;
}

}  // namespace vmp