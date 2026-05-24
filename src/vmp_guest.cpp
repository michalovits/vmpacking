#include <vmp_guest.h>

#include <algorithm>
#include <ostream>
#include <vmp_host.h>

namespace vmp
{

Guest::Guest(const std::unordered_set<int> &pages) : pages(pages) {}

size_t Guest::getUniquePageCount() const
{
    return pages.size();
}

size_t Guest::countUniquePagesOn(const Host &host) const
{
    return std::ranges::count_if(pages,
                                 [&](const int page) { return host.getPageFrequency(page); });
}

std::ostream &operator<<(std::ostream &os, const Guest &guest)
{
    os << "Guest{ [";
    if (!guest.pages.empty()) {
        auto it = guest.pages.begin();
        os << *it;
        while (++it != guest.pages.end()) {
            os << ", " << *it;
        }
    }
    os << "] (len=" << guest.getUniquePageCount() << ") }";
    return os;
}

}  // namespace vmp