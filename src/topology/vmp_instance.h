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
        guestPtrs_.reserve(guests_.size());
        for (const auto &guest : guests_) {
            guestPtrs_.push_back(&guest);
        }
    }

    Instance(const Instance &) = delete;
    Instance(Instance &&) = default;

    Instance &operator=(const Instance &) = delete;
    Instance &operator=(Instance &&) = default;

    [[nodiscard]] std::span<const Guest *const> guests() const { return guestPtrs_; }
    [[nodiscard]] size_t guestCount() const { return guests_.size(); }
    [[nodiscard]] size_t capacity() const { return capacity_; }

  private:
    size_t capacity_;

    std::vector<Guest> guests_;
    std::vector<const Guest *> guestPtrs_;
};

std::ostream &operator<<(std::ostream &os, const Instance &instance);

}  // namespace vmp

#endif  // VMP_INSTANCE_H
