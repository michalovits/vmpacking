#ifndef SOLVERS_PACKING_H
#define SOLVERS_PACKING_H

#include <vmp_host.h>

#include <algorithm>
#include <memory>
#include <ostream>
#include <unordered_set>
#include <vector>

namespace vmp
{

enum PackingValidity
{
    PACKING_OKAY = 0,
    PACKING_HOST_OVERFULL,
    PACKING_HOST_EMPTY,
    PACKING_PARTIAL
};

inline std::ostream &operator<<(std::ostream &os, const PackingValidity validity)
{
    switch (validity) {
        case PACKING_OKAY:
            os << "PACKING_OKAY";
            break;
        case PACKING_HOST_OVERFULL:
            os << "PACKING_HOST_OVERFULL";
            break;
        case PACKING_HOST_EMPTY:
            os << "PACKING_HOST_EMPTY";
            break;
        case PACKING_PARTIAL:
            os << "PACKING_PARTIAL";
            break;
    }
    return os;
}

class Packing
{
  public:
    explicit Packing(std::vector<std::shared_ptr<Host>> hosts);

    Packing(Packing &other) noexcept = default;
    Packing(Packing &&other) noexcept = default;

    Packing &operator=(Packing &&other) noexcept = default;

    /**
     * Validate the packing against an instance, by checking that:
     * - All guests are present,
     * - No host is overfull; and
     * - No host is empty
     *
     * @param instance the instance against which to validate
     * @return
     */
    template <typename InstanceType>
        requires Instance<InstanceType>
    PackingValidity validateForInstance(const InstanceType &instance) const
    {
        std::unordered_set<std::shared_ptr<const Guest>> placedGuests;

        for (const auto &host : hosts_) {
            if (host->guests().empty()) {
                return PACKING_HOST_EMPTY;
            }
            if (host->isOverfull()) {
                return PACKING_HOST_OVERFULL;
            }
            for (const auto &guest : host->guests()) {
                placedGuests.insert(guest);
            }
        }

        const auto &guests = instance.guests();
        const auto isPlaced = [&](const auto &guest) {
            return placedGuests.contains(guest);
        };
        if (!std::ranges::all_of(guests, isPlaced)) {
            return PACKING_PARTIAL;
        }

        return PACKING_OKAY;
    }

    void decantGuests();

    void addHost(const std::shared_ptr<Host> &host);

    [[nodiscard]] size_t guestCount() const;
    [[nodiscard]] size_t hostCount() const;

    [[nodiscard]] std::vector<std::shared_ptr<Host>> &hosts();

  private:
    std::vector<std::shared_ptr<Host>> hosts_;
    size_t guestCount_;
};

}  // namespace vmp
#endif  // SOLVERS_PACKING_H