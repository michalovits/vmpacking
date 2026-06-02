#ifndef VMP_TESTS_INSTANCE_BUILDERS_H
#define VMP_TESTS_INSTANCE_BUILDERS_H

#include <vmp_guest.h>
#include <vmp_instance.h>

#include <deque>
#include <initializer_list>
#include <unordered_set>
#include <vector>

namespace vmp::testing
{

/// Returns a `const Guest*` to a guest with stable address for the lifetime of the test binary.
/// Backed by a function-local `std::deque`, which never relocates its elements, so the pointers
/// remain valid (test-only convenience for standalone guests not owned by an Instance).
inline const Guest *makeGuest(std::initializer_list<int> pages)
{
    static std::deque<Guest> pool;
    pool.emplace_back(std::unordered_set(pages));
    return &pool.back();
}

inline std::vector<const Guest *>
makeGuests(std::initializer_list<std::initializer_list<int>> guestPages)
{
    std::vector<const Guest *> guests;
    guests.reserve(guestPages.size());

    for (const auto &pages : guestPages) {
        guests.push_back(makeGuest(pages));
    }
    return guests;
}

inline Instance makeInstance(size_t capacity,
                             std::initializer_list<std::initializer_list<int>> guestPages)
{
    std::vector<Guest> guests;
    guests.reserve(guestPages.size());
    for (const auto &pages : guestPages) {
        guests.emplace_back(std::unordered_set(pages));
    }
    return Instance(capacity, std::move(guests));
}

}  // namespace vmp::testing

#endif  // VMP_TESTS_INSTANCE_BUILDERS_H
