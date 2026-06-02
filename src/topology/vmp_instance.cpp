#include <vmp_instance.h>

#include <ostream>

namespace vmp
{

std::ostream &operator<<(std::ostream &os, const Instance &instance)
{
    os << "Instance{ capacity=" << instance.capacity() << ", guests=[";

    bool first = true;
    for (const Guest *guest : instance.guests()) {
        if (!first)
            os << ", ";
        first = false;

        os << "{";
        const auto &pages = guest->pages;
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
