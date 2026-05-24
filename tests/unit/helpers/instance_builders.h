#ifndef VMP_TESTS_INSTANCE_BUILDERS_H
#define VMP_TESTS_INSTANCE_BUILDERS_H

#include <vmp_generalinstance.h>
#include <vmp_guest.h>

#include <initializer_list>
#include <memory>
#include <unordered_set>
#include <vector>

namespace vmp::testing
{

inline std::shared_ptr<const Guest> makeGuest(std::initializer_list<int> pages)
{
    return std::make_shared<const Guest>(std::unordered_set(pages));
}

inline std::vector<std::shared_ptr<const Guest>>
makeGuests(std::initializer_list<std::initializer_list<int>> guestPages)
{
    std::vector<std::shared_ptr<const Guest>> guests;
    guests.reserve(guestPages.size());

    for (const auto &pages : guestPages) {
        guests.push_back(makeGuest(pages));
    }
    return guests;
}

inline GeneralInstance
makeGeneralInstance(size_t capacity, std::initializer_list<std::initializer_list<int>> guestPages)
{
    return GeneralInstance(capacity, makeGuests(guestPages));
}

}  // namespace vmp::testing

#endif  // VMP_TESTS_INSTANCE_BUILDERS_H
