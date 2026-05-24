#ifndef SOLVERS_INSTANCE_H
#define SOLVERS_INSTANCE_H

#include <memory>
#include <vector>
#include <vmp_guest.h>

namespace vmp
{

class GeneralInstance
{
  public:
    GeneralInstance(size_t capacity, std::vector<std::shared_ptr<const Guest>> guests);

    [[nodiscard]] const std::vector<std::shared_ptr<const Guest>> &guests() const;
    [[nodiscard]] size_t guestCount() const;
    [[nodiscard]] size_t capacity() const;

    friend std::ostream &operator<<(std::ostream &os, const GeneralInstance &instance);

  private:
    const size_t capacity_;
    const std::vector<std::shared_ptr<const Guest>> guests_;
};

}  // namespace vmp

#endif  // SOLVERS_INSTANCE_H