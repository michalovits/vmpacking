#ifndef SOLVERS_GUEST_H
#define SOLVERS_GUEST_H

#include <iosfwd>
#include <unordered_set>

namespace vmp
{

class Host;

class Guest
{
  public:
    explicit Guest(const std::unordered_set<int> &pages);
    [[nodiscard]] size_t getUniquePageCount() const;
    [[nodiscard]] size_t countUniquePagesOn(const Host &host) const;

    const std::unordered_set<int> pages;

    friend std::ostream &operator<<(std::ostream &os, const Guest &guest);
};

}  // namespace vmp

#endif  // SOLVERS_GUEST_H