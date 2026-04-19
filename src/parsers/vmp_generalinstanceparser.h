#ifndef SOLVERS_INSTANCELOADER_H
#define SOLVERS_INSTANCELOADER_H

#include <vmp_generalinstance.h>

#include <json.hpp>
#include <set>
#include <vector>

namespace vmp
{

class GeneralInstanceParser
{
  public:
    explicit GeneralInstanceParser(std::string directory, std::string capacityName = "capacity",
                                   std::string guestsName = "guests");

    [[nodiscard]] std::vector<GeneralInstance> load(int maxInstances = -1);

    ~GeneralInstanceParser() = default;

  private:
    const std::string directory;

    const std::string capacityName;
    const std::string guestsName;

    std::set<std::filesystem::path> paths;
    std::unordered_map<std::filesystem::path, int> processedInstances;
};

}  // namespace vmp

#endif  // SOLVERS_INSTANCELOADER_H