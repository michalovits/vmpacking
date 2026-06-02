#include <vmp_instance.h>

#include <ostream>

namespace vmp
{

std::span<const Guest> Instance::guests() const
{
    return guests_;
}

size_t Instance::guestCount() const
{
    return guests_.size();
}

size_t Instance::capacity() const
{
    return capacity_;
}

std::ostream &operator<<(std::ostream &os, const Instance &instance)
{
    os << "Instance{ capacity=" << instance.capacity() << ", guests=[";

    bool first = true;
    for (const Guest &guest : instance.guests()) {
        if (!first) {
            os << ", ";
        }
        first = false;

        os << "{";
        const auto &ps = guest.pages;
        for (auto it = ps.begin(); it != ps.end(); ++it) {
            if (it != ps.begin())
                os << ",";
            os << *it;
        }
        os << "}";
    }

    os << "] }";
    return os;
}

}  // namespace vmp
