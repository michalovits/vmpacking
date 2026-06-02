#ifndef VMP_INSTANCEPARSER_H
#define VMP_INSTANCEPARSER_H

#include <vmp_instance.h>

#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <unordered_map>
#include <vector>

namespace vmp
{

class InstanceParser
{
  public:
    explicit InstanceParser(std::string directory, std::string capacityName = "capacity",
                            std::string guestsName = "guests");

    [[nodiscard]] std::vector<Instance>
    load(size_t maxInstances = std::numeric_limits<size_t>::max());

  private:
    const std::string directory;

    const std::string capacityName;
    const std::string guestsName;

    std::set<std::filesystem::path> paths;
    std::unordered_map<std::filesystem::path, size_t> processedInstances;
};

}  // namespace vmp

#endif  // VMP_INSTANCEPARSER_H
