#include <vmp_host.h>

#include <ostream>
#include <vmp_guest.h>

namespace vmp
{

Host::Host(const size_t capacity) : capacity(capacity) {}

bool Host::addGuest(const std::shared_ptr<const Guest> &guest)
{
    for (const int page : guest->pages) {
        ++pageFrequencies[page];
    }
    guests.insert(guest);

    return isOverfull();
}

bool Host::removeGuest(const std::shared_ptr<const Guest> &guest)
{
    for (const int page : guest->pages) {
        if (--pageFrequencies[page] == 0) {
            pageFrequencies.erase(page);
        }
    }

    guests.erase(guest);

    return isOverfull();
}

void Host::clearGuests()
{
    guests.clear();
    pageFrequencies.clear();
}

size_t Host::getCapacity() const
{
    return capacity;
}

const std::unordered_set<std::shared_ptr<const Guest>> &Host::getGuests() const
{
    return guests;
}

bool Host::accommodatesGuest(const Guest &guest) const
{
    return countPagesWithGuest(guest) <= capacity;
}

const std::unordered_map<int, int> &Host::getPageFrequencies() const
{
    return pageFrequencies;
}

size_t Host::getPageFrequency(const int page) const
{
    return pageFrequencies.contains(page) ? pageFrequencies.at(page) : 0;
}

size_t Host::getUniquePageCount() const
{
    return pageFrequencies.size();
}

size_t Host::countPagesWithGuest(const Guest &guest) const
{
    return getUniquePageCount() + guest.getUniquePageCount() - guest.countUniquePagesOn(*this);
}

size_t Host::countPagesNotOn(const Guest &guest) const
{
    return getUniquePageCount() - guest.countUniquePagesOn(*this);
}

size_t Host::getGuestCount() const
{
    return guests.size();
}

bool Host::isOverfull() const
{
    return pageFrequencies.size() > capacity;
}

bool Host::hasGuest(const std::shared_ptr<const Guest> &guest) const
{
    return guests.contains(guest);
}

std::ostream &operator<<(std::ostream &os, const Host &host)
{
    os << "Host{ capacity=" << host.capacity << ", [";

    const auto &guests = host.guests;
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

    os << "] (len: " << host.getGuestCount() << ") }";
    return os;
}

}  // namespace vmp