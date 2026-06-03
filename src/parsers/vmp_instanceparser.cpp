#include <vmp_instanceparser.h>

#include <filesystem>
#include <fstream>
#include <vector>

using json = nlohmann::json;

namespace vmp
{

InstanceParser::InstanceParser(std::string directory, std::string capacityName,
                               std::string guestsName)
    : directory(std::move(directory)),
      capacityName(std::move(capacityName)),
      guestsName(std::move(guestsName))
{
}

static std::vector<Instance>
makeInstances(const std::vector<int> &capacityData,
              const std::vector<std::vector<std::vector<int>>> &guestData)
{
    assert(capacityData.size() == guestData.size());

    std::vector<Instance> instances;
    for (size_t i = 0; i < capacityData.size(); ++i) {
        std::vector<Guest> guests;
        guests.reserve(guestData[i].size());

        for (const auto &guestPages : guestData[i]) {
            guests.emplace_back(std::unordered_set(guestPages.begin(), guestPages.end()));
        }
        instances.emplace_back(capacityData[i], std::move(guests));
    }

    return instances;
}

std::vector<Instance> InstanceParser::load(const size_t maxInstances)
{
    namespace fs = std::filesystem;

    std::vector<Instance> instances;
    std::vector<int> capacityData;
    std::vector<std::vector<std::vector<int>>> guestData;

    for (const auto &directoryEntry : fs::directory_iterator(directory)) {
        if (directoryEntry.path().extension() == ".json") {
            paths.emplace(directoryEntry);
        }
    }

    while (!paths.empty()) {
        const auto path = *paths.begin();
        paths.erase(path);

        auto file = std::ifstream(path);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file " + path.string());
        }

        if (!processedInstances.contains(path)) {
            processedInstances[path] = 0;
        }

        const auto rootNodesJson = json::parse(file);

        for (size_t i = processedInstances[path]; i < rootNodesJson.size(); ++i) {
            const auto &instanceJson = rootNodesJson[i];

            capacityData.push_back(instanceJson[capacityName].get<int>());
            guestData.push_back(instanceJson[guestsName].get<std::vector<std::vector<int>>>());

            ++processedInstances[path];

            if (guestData.size() == maxInstances) {
                return makeInstances(capacityData, guestData);
            }
        }
    }

    return makeInstances(capacityData, guestData);
}

}  // namespace vmp