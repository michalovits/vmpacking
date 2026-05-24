#include <vmp_host.h>

#include <ostream>
#include <vmp_guest.h>

namespace vmp
{

Host::Host(const size_t capacity) : capacity_(capacity) {}

bool Host::addGuest(const std::shared_ptr<const Guest> &guest)
{
    for (const int page : guest->pages) {
        ++pageFrequencies_[page];
    }
    guests_.insert(guest);

    return isOverfull();
}

bool Host::removeGuest(const std::shared_ptr<const Guest> &guest)
{
    for (const int page : guest->pages) {
        if (--pageFrequencies_[page] == 0) {
            pageFrequencies_.erase(page);
        }
    }

    guests_.erase(guest);

    return isOverfull();
}

void Host::clearGuests()
{
    guests_.clear();
    pageFrequencies_.clear();
}

size_t Host::capacity() const
{
    return capacity_;
}

const std::unordered_set<std::shared_ptr<const Guest>> &Host::guests() const
{
    return guests_;
}

bool Host::accommodatesGuest(const Guest &guest) const
{
    return countPagesWithGuest(guest) <= capacity_;
}

const std::unordered_map<int, int> &Host::pageFrequencies() const
{
    return pageFrequencies_;
}

size_t Host::pageFrequency(const int page) const
{
    return pageFrequencies_.contains(page) ? pageFrequencies_.at(page) : 0;
}

size_t Host::uniquePageCount() const
{
    return pageFrequencies_.size();
}

size_t Host::countPagesWithGuest(const Guest &guest) const
{
    return uniquePageCount() + guest.uniquePageCount() - guest.countUniquePagesOn(*this);
}

size_t Host::countPagesNotOn(const Guest &guest) const
{
    return uniquePageCount() - guest.countUniquePagesOn(*this);
}

size_t Host::guestCount() const
{
    return guests_.size();
}

bool Host::isOverfull() const
{
    return pageFrequencies_.size() > capacity_;
}

bool Host::hasGuest(const std::shared_ptr<const Guest> &guest) const
{
    return guests_.contains(guest);
}

std::ostream &operator<<(std::ostream &os, const Host &host)
{
    os << "Host{ capacity=" << host.capacity_ << ", [";

    const auto &guests = host.guests_;
    for (auto it = guests.begin(); it != guests.end(); ++it) {
        if (it != guests.begin()) {
            os << ", ";
        }
        if (*it == nullptr) {
            os << "NULL";
            continue;
        }
        os << **it;
    }

    os << "] (len: " << host.guestCount() << ") }";
    return os;
}

}  // namespace vmp