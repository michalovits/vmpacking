#ifndef VMP_GENERALTOPOLOGY_H
#define VMP_GENERALTOPOLOGY_H

#include <vmp_guest.h>
#include <vmp_instance.h>

#include <iosfwd>
#include <memory>
#include <vector>

namespace vmp
{

class GeneralTopology
{
  public:
    explicit GeneralTopology(std::vector<std::shared_ptr<const Guest>> guests);

    [[nodiscard]] const std::vector<std::shared_ptr<const Guest>> &guests() const;
    [[nodiscard]] size_t guestCount() const;

  private:
    std::vector<std::shared_ptr<const Guest>> guests_;
};

using GeneralInstance = Instance<GeneralTopology>;

std::ostream &operator<<(std::ostream &os, const GeneralInstance &instance);

}  // namespace vmp

#endif  // VMP_GENERALTOPOLOGY_H
