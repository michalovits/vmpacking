#include <vmp_clustertreeloader.h>

#include <cassert>
#include <fstream>
#include <string>

using json = nlohmann::json;

namespace vmp
{

ClusterTreeLoader::ClusterTreeLoader(std::string directory, std::string capacityName,
                                     std::string nodesName, std::string nodeIdName,
                                     std::string nodeParentsName, std::string pagesName,
                                     std::string guestPagesName, std::string clusterChildrenName)
    : directory_(std::move(directory)),
      capacityName_(std::move(capacityName)),
      nodesName_(std::move(nodesName)),
      nodeIdName_(std::move(nodeIdName)),
      nodeParentsName_(std::move(nodeParentsName)),
      pagesName_(std::move(pagesName)),
      guestPagesName_(std::move(guestPagesName)),
      clusterChildrenName_(std::move(clusterChildrenName))
{
}

std::optional<Guest> ClusterTreeLoader::parseGuest(const json &nodeJson) const
{
    if (!nodeJson.contains(guestPagesName_)) {
        return std::nullopt;
    }
    return Guest(std::unordered_set<int>(nodeJson[guestPagesName_].begin(),
                                         nodeJson[guestPagesName_].end()));
}

void ClusterTreeLoader::parseClusterSubtree(ClusterTreeBuilder &builder, const size_t parentCluster,
                                            const json &clusterJson,
                                            std::unordered_map<size_t, size_t> &fromJsonNode,
                                            const bool skipRoot) const
{
    // Link directly to the sentinel or create the root
    const size_t cluster = skipRoot ? builder.rootCluster() : builder.createCluster(parentCluster);

    if (clusterJson.contains(nodesName_)) {
        for (const auto &nodeJson : clusterJson[nodesName_]) {
            auto pages = nodeJson[pagesName_].get<std::unordered_set<int>>();
            const size_t jsonNodeId = nodeJson[nodeIdName_].get<size_t>();
            std::vector<size_t> parents;

            for (const size_t jsonNodeParent : nodeJson[nodeParentsName_].get<std::vector<int>>()) {
                parents.push_back(fromJsonNode.at(jsonNodeParent));
            }

            const size_t node =
                nodeJson.contains(guestPagesName_)
                    ? builder.addLeafNode(std::move(parents), *parseGuest(nodeJson),
                                          std::move(pages))
                    : builder.addInnerNode(cluster, std::move(parents), std::move(pages));

            fromJsonNode[jsonNodeId] = node;
        }
    }

    for (const auto &clusterChildJson : clusterJson[clusterChildrenName_]) {
        parseClusterSubtree(builder, cluster, clusterChildJson, fromJsonNode, false);
    }
}

std::vector<ClusterTree> ClusterTreeLoader::load(const size_t maxInstances)
{
    namespace fs = std::filesystem;

    std::vector<ClusterTree> instances;

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

            std::unordered_map<size_t, size_t> jsonToNodeIds;

            auto builder = ClusterTreeBuilder(capacity);
            parseClusterSubtree(builder, builder.rootCluster(), rootNodeJson, jsonToNodeIds, true);

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
