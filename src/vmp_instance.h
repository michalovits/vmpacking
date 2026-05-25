#ifndef VMP_INSTANCE_H
#define VMP_INSTANCE_H

#include <cstddef>
#include <utility>

namespace vmp
{

template <typename Topology>
class Instance
{
  public:
    template <typename... TopologyArgs>
    explicit Instance(const size_t capacity, TopologyArgs &&...topologyArgs)
        : topology_(std::forward<TopologyArgs>(topologyArgs)...), capacity_(capacity)
    {
    }

    [[nodiscard]] const Topology &topology() const { return topology_; }

    [[nodiscard]] decltype(auto) guests() const { return topology_.guests(); }
    [[nodiscard]] size_t guestCount() const { return topology_.guestCount(); }
    [[nodiscard]] size_t capacity() const { return capacity_; }

  private:
    Topology topology_;
    size_t capacity_;
};

}  // namespace vmp

#endif  // VMP_INSTANCE_H
