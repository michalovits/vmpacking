#include <vmp_generalinstance.h>

#include <ostream>

namespace vmp
{

GeneralInstance::GeneralInstance(const size_t capacity,
                                 std::vector<std::shared_ptr<const Guest>> guests)
    : capacity_(capacity), guests_(std::move(guests))
{
}

size_t GeneralInstance::guestCount() const
{
    return guests_.size();
}

const std::vector<std::shared_ptr<const Guest>> &GeneralInstance::guests() const
{
    return guests_;
}

size_t GeneralInstance::capacity() const
{
    return capacity_;
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