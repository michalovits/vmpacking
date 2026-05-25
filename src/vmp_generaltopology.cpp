#include <vmp_generaltopology.h>

#include <ostream>

namespace vmp
{

GeneralTopology::GeneralTopology(std::vector<std::shared_ptr<const Guest>> guests)
    : guests_(std::move(guests))
{
}

const std::vector<std::shared_ptr<const Guest>> &GeneralTopology::guests() const
{
    return guests_;
}

size_t GeneralTopology::guestCount() const
{
    return guests_.size();
}

std::ostream &operator<<(std::ostream &os, const GeneralInstance &instance)
{
    os << "Instance{ capacity=" << instance.capacity() << ", guests=[";

    const auto &guests = instance.guests();
    for (size_t i = 0; i < guests.size(); ++i) {
        if (i > 0)
            os << ", ";

        os << "{";
        const auto &pages = guests[i]->pages;
        for (auto it = pages.begin(); it != pages.end(); ++it) {
            if (it != pages.begin())
                os << ",";
            os << *it;
        }
        os << "}";
    }

    os << "] }";
    return os;
}

}  // namespace vmp
