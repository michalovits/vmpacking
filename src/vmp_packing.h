#ifndef SOLVERS_PACKING_H
#define SOLVERS_PACKING_H

#include <vmp_host.h>

#include <ostream>
#include <unordered_set>

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
        for (const auto &host : hosts) {
            if (host->getGuests().empty()) {
                return PACKING_HOST_EMPTY;
            }
            if (host->isOverfull()) {
                return PACKING_HOST_OVERFULL;
            }
            for (const auto &guest : host->getGuests()) {
                placedGuests.insert(guest);
            }
        }

        const auto &guests = instance.getGuests();
        if (!std::ranges::all_of(guests.begin(), guests.end(),
                                 [&](const auto &guest) { return placedGuests.contains(guest); })) {
            return PACKING_PARTIAL;
        }

        return PACKING_OKAY;
    }

    void decantGuests();

    void addHost(const std::shared_ptr<Host> &host);

    [[nodiscard]] size_t getGuestCount() const;
    [[nodiscard]] size_t getHostCount() const;

    [[nodiscard]] std::vector<std::shared_ptr<Host>> &getHosts();

  private:
    std::vector<std::shared_ptr<Host>> hosts;
    size_t guestCount;
};

}  // namespace vmp
#endif  // SOLVERS_PACKING_H