#include <vmp_generalinstanceparser.h>

#include <filesystem>
#include <fstream>
#include <vector>

using json = nlohmann::json;

namespace vmp
{

GeneralInstanceParser::GeneralInstanceParser(std::string directory, std::string capacityName,
                                             std::string guestsName)
    : directory(std::move(directory)),
      capacityName(std::move(capacityName)),
      guestsName(std::move(guestsName))
{
}

static std::vector<GeneralInstance>
makeInstances(const std::vector<int> &capacityData,
              const std::vector<std::vector<std::vector<int>>> &guestData)
{
    assert(capacityData.size() == guestData.size());

    std::vector<GeneralInstance> instances;
    for (int i = 0; i < capacityData.size(); ++i) {
        std::vector<std::shared_ptr<const Guest>> guests;
        for (const auto &guestPages : guestData[i]) {
            guests.push_back(
                std::make_shared<Guest>(std::unordered_set(guestPages.begin(), guestPages.end())));
        }
        instances.emplace_back(capacityData[i], std::move(guests));
    }

    return instances;
}

std::vector<GeneralInstance> GeneralInstanceParser::load(const int maxInstances)
{
    namespace fs = std::filesystem;

    std::vector<GeneralInstance> instances;
    std::vector<int> capacityData;
    std::vector<std::vector<std::vector<int>>> guestData;

    instances.reserve(maxInstances);
    capacityData.reserve(maxInstances);
    guestData.reserve(maxInstances);

    for (const auto &directoryEntry : fs::directory_iterator(directory)) {
        if (directoryEntry.path().extension() == ".json") {
            paths.emplace(directoryEntry);
        }
    }

    while (!paths.empty()) {
        const auto path = *paths.begin();
        paths.erase(path);

        std::ifstream file(path);
        assert(file.is_open());

        if (!processedInstances.contains(path)) {
            processedInstances[path] = 0;
        }

        const auto rootNodesJson = json::parse(file);

        for (int i = processedInstances[path]; i < rootNodesJson.size(); ++i) {
            const auto &instanceJson = rootNodesJson[i];
            assert(instanceJson.contains(capacityName));
            assert(instanceJson.contains(guestsName));

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