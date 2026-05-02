#ifndef SOLVERS_INSTANCE_H
#define SOLVERS_INSTANCE_H

#include <vector>
#include <vmp_guest.h>

namespace vmp
{

class GeneralInstance
{
  public:
    GeneralInstance(size_t capacity, std::vector<std::shared_ptr<const Guest>> guests);

    [[nodiscard]] size_t getGuestCount() const;

    friend std::ostream &operator<<(std::ostream &os, const GeneralInstance &instance);

    [[nodiscard]] const std::vector<std::shared_ptr<const Guest>> &getGuests() const;
    [[nodiscard]] size_t getCapacity() const;

  private:
    const size_t capacity;
    const std::vector<std::shared_ptr<const Guest>> guests;
};

}  // namespace vmp

#endif  // SOLVERS_INSTANCE_H