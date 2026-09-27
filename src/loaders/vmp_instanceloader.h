#ifndef VMP_INSTANCELOADER_H
#define VMP_INSTANCELOADER_H

#include <vmp_instance.h>

#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <unordered_map>
#include <vector>

namespace vmp
{

class InstanceLoader
{
  public:
    explicit InstanceLoader(std::string directory, std::string capacityName = "capacity",
                            std::string guestsName = "guests");

    [[nodiscard]] std::vector<Instance>
    load(size_t maxInstances = std::numeric_limits<size_t>::max());

  private:
    const std::string directory_;

    const std::string capacityName_;
    const std::string guestsName_;

    std::set<std::filesystem::path> paths_;
    std::unordered_map<std::filesystem::path, size_t> processedInstances_;
};

}  // namespace vmp

#endif  // VMP_INSTANCELOADER_H
