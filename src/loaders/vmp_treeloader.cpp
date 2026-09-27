#include <unordered_set>
#include <vmp_treeloader.h>

#include <cassert>
#include <fstream>
#include <string>

using json = nlohmann::json;

namespace vmp
{

TreeLoader::TreeLoader(std::string directory, std::string capacityName, std::string guestPagesName,
                       std::string pagesName, std::string childrenName)
    : directory_(std::move(directory)),
      capacityName_(std::move(capacityName)),
      guestPagesName_(std::move(guestPagesName)),
      pagesName_(std::move(pagesName)),
      childrenName_(std::move(childrenName))
{
}

std::optional<Guest> TreeLoader::parseGuest(const json &nodeJson) const
{
    if (!nodeJson.contains(guestPagesName_)) {
        return std::nullopt;
    }
    return Guest(std::unordered_set<int>(nodeJson[guestPagesName_].begin(),
                                         nodeJson[guestPagesName_].end()));
}

void TreeLoader::parseChildren(TreeBuilder &builder, const size_t parent,
                               const json &nodeJson) const
{
    for (const auto &childJson : nodeJson[childrenName_]) {
        auto childPages = childJson[pagesName_].get<std::unordered_set<int>>();

        const size_t child =
            childJson.contains(guestPagesName_)
                ? builder.addLeafNode(parent, *parseGuest(childJson), std::move(childPages))
                : builder.addInnerNode(parent, std::move(childPages));

        if (childJson.contains(childrenName_)) {
            parseChildren(builder, child, childJson);
        }
    }
};

std::vector<Tree> TreeLoader::load(const size_t maxInstances)
{
    namespace fs = std::filesystem;

    std::vector<Tree> instances;

    for (const auto &directoryEntry : fs::directory_iterator(directory_)) {
        if (directoryEntry.path().extension() == ".json") {
            paths_.emplace(directoryEntry);
        }
    }

    while (!paths_.empty()) {
        const auto path = *paths_.begin();
        paths_.erase(path);

        auto file = std::ifstream(path);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file " + path.string());
        }

        if (!processedInstances_.contains(path)) {
            processedInstances_[path] = 0;
        }

        const auto rootNodesJson = json::parse(file);

        for (size_t i = processedInstances_[path]; i < rootNodesJson.size(); ++i) {
            const auto &rootNodeJson = rootNodesJson[i];
            const size_t capacity = rootNodeJson[capacityName_].get<size_t>();

            auto rootGuest = parseGuest(rootNodeJson);
            auto rootPages = std::unordered_set<int>{};

            if (rootGuest.has_value()) {
                rootPages = rootNodeJson[pagesName_].get<std::unordered_set<int>>();
            }

            auto builder = TreeBuilder(capacity, std::move(rootPages));

            if (rootGuest.has_value()) {
                builder.addLeafNode(builder.rootNode(), std::move(*rootGuest),
                                    std::move(rootPages));
            }

            if (rootNodeJson.contains(childrenName_)) {
                parseChildren(builder, builder.rootNode(), rootNodeJson);
            }

            builder.setLabel(path.filename().string() + "#" + std::to_string(i));

            instances.push_back(std::move(builder).build());
            ++processedInstances_[path];

            if (instances.size() == maxInstances) {
                return instances;
            }
        }
    }

    return instances;
}

}  // namespace vmp