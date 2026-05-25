#include <vmp_treeinstanceparser.h>

#include <cassert>
#include <fstream>

using json = nlohmann::json;

namespace vmp
{

TreeInstanceParser::TreeInstanceParser(std::string directory, std::string capacityName,
                                       std::string guestPagesName, std::string pagesName,
                                       std::string childrenName)
    : directory(std::move(directory)),
      capacityName(std::move(capacityName)),
      guestPagesName(std::move(guestPagesName)),
      pagesName(std::move(pagesName)),
      childrenName(std::move(childrenName))
{
}

std::shared_ptr<Guest> TreeInstanceParser::parseGuest(const json &nodeJson) const
{
    if (!nodeJson.contains(guestPagesName)) {
        return nullptr;
    }
    return std::make_shared<Guest>(
        std::unordered_set<int>(nodeJson[guestPagesName].begin(), nodeJson[guestPagesName].end()));
}

void TreeInstanceParser::parseChildren(TreeInstanceBuilder &builder, const size_t parent,
                                       const json &nodeJson) const
{
    for (const auto &childJson : nodeJson[childrenName]) {
        std::unordered_set<int> childPages = childJson[pagesName].get<std::unordered_set<int>>();

        const size_t child =
            childJson.contains(guestPagesName)
                ? builder.addLeafNode(parent, parseGuest(childJson), std::move(childPages))
                : builder.addInnerNode(parent, std::move(childPages));

        if (childJson.contains(childrenName)) {
            parseChildren(builder, child, childJson);
        }
    }
};

std::vector<TreeInstance> TreeInstanceParser::load(const size_t maxInstances)
{
    namespace fs = std::filesystem;

    std::vector<TreeInstance> instances;

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
            const auto rootGuest = parseGuest(rootNodeJson);
            auto rootPages = rootNodeJson[pagesName].get<std::unordered_set<int>>();

            TreeInstanceBuilder builder(capacity, rootGuest != nullptr ? std::unordered_set<int>{}
                                                                       : std::move(rootPages));
            if (rootGuest != nullptr) {
                builder.addLeafNode(builder.rootNode(), rootGuest, std::move(rootPages));
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