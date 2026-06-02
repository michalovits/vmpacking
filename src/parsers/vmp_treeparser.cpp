#include <vmp_treeparser.h>

#include <cassert>
#include <fstream>

using json = nlohmann::json;

namespace vmp
{

TreeParser::TreeParser(std::string directory, std::string capacityName, std::string guestPagesName,
                       std::string pagesName, std::string childrenName)
    : directory(std::move(directory)),
      capacityName(std::move(capacityName)),
      guestPagesName(std::move(guestPagesName)),
      pagesName(std::move(pagesName)),
      childrenName(std::move(childrenName))
{
}

std::optional<Guest> TreeParser::parseGuest(const json &nodeJson) const
{
    if (!nodeJson.contains(guestPagesName)) {
        return std::nullopt;
    }
    return Guest(
        std::unordered_set<int>(nodeJson[guestPagesName].begin(), nodeJson[guestPagesName].end()));
}

void TreeParser::parseChildren(TreeBuilder &builder, const size_t parent,
                               const json &nodeJson) const
{
    for (const auto &childJson : nodeJson[childrenName]) {
        std::unordered_set<int> childPages = childJson[pagesName].get<std::unordered_set<int>>();

        const size_t child =
            childJson.contains(guestPagesName)
                ? builder.addLeafNode(parent, *parseGuest(childJson), std::move(childPages))
                : builder.addInnerNode(parent, std::move(childPages));

        if (childJson.contains(childrenName)) {
            parseChildren(builder, child, childJson);
        }
    }
};

std::vector<Tree> TreeParser::load(const size_t maxInstances)
{
    namespace fs = std::filesystem;

    std::vector<Tree> instances;

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

        for (size_t i = processedInstances[path]; i < rootNodesJson.size(); ++i) {
            const auto &rootNodeJson = rootNodesJson[i];
            assert(rootNodeJson.contains(capacityName));

            const size_t capacity = rootNodeJson[capacityName].get<size_t>();
            auto rootGuest = parseGuest(rootNodeJson);
            auto rootPages = rootNodeJson[pagesName].get<std::unordered_set<int>>();

            TreeBuilder builder(capacity, rootGuest.has_value() ? std::unordered_set<int>{}
                                                                : std::move(rootPages));
            if (rootGuest.has_value()) {
                builder.addLeafNode(builder.rootNode(), std::move(*rootGuest),
                                    std::move(rootPages));
            }

            if (rootNodeJson.contains(childrenName)) {
                parseChildren(builder, builder.rootNode(), rootNodeJson);
            }

            instances.push_back(std::move(builder).build());
            ++processedInstances[path];

            if (instances.size() == maxInstances) {
                return instances;
            }
        }
    }

    return instances;
}

}  // namespace vmp