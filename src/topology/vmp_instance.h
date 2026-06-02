#ifndef VMP_INSTANCE_H
#define VMP_INSTANCE_H

#include <vmp_guest.h>

#include <cstddef>
#include <iosfwd>
#include <span>
#include <utility>
#include <vector>

namespace vmp
{

class Instance
{
  public:
    Instance(const size_t capacity, std::vector<Guest> guests)
        : capacity_(capacity), guests_(std::move(guests))
    {
    }

    Instance(const Instance &) = delete;
    Instance(Instance &&) = default;

    Instance &operator=(const Instance &) = delete;
    Instance &operator=(Instance &&) = default;

    [[nodiscard]] std::span<const Guest> guests() const;
    [[nodiscard]] size_t guestCount() const;
    [[nodiscard]] size_t capacity() const;

  private:
    size_t capacity_;
    std::vector<Guest> guests_;
};

std::ostream &operator<<(std::ostream &os, const Instance &instance);

}  // namespace vmp

#endif  // VMP_INSTANCE_H
