#include <vmp_clustertreeinstanceparser.h>

#include <cassert>
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

namespace vmp
{

ClusterTreeInstanceParser::ClusterTreeInstanceParser(
    std::string directory, std::string capacityName, std::string nodesName, std::string nodeIdName,
    std::string nodeParentsName, std::string pagesName, std::string guestPagesName,
    std::string clusterChildrenName)
    : directory(std::move(directory)),
      capacityName(std::move(capacityName)),
      nodesName(std::move(nodesName)),
      nodeIdName(std::move(nodeIdName)),
      nodeParentsName(std::move(nodeParentsName)),
      pagesName(std::move(pagesName)),
      guestPagesName(std::move(guestPagesName)),
      clusterChildrenName(std::move(clusterChildrenName))
{
}

std::shared_ptr<Guest> ClusterTreeInstanceParser::parseGuest(const json &nodeJson) const
{
    if (!nodeJson.contains(guestPagesName)) {
        return nullptr;
    }
    return std::make_shared<Guest>(
        std::unordered_set<int>(nodeJson[guestPagesName].begin(), nodeJson[guestPagesName].end()));
}

void ClusterTreeInstanceParser::parseClusterSubtree(
    ClusterTreeInstance &instance, const size_t parentCluster, const json &clusterJson,
    std::unordered_map<size_t, size_t> &fromJsonNode, const bool skipRoot) const
{
    const size_t cluster =
        skipRoot ? ClusterTreeInstance::getRootCluster() : instance.createCluster(parentCluster);

    if (clusterJson.contains(nodesName)) {
        for (const auto &nodeJson : clusterJson[nodesName]) {
            const std::unordered_set<int> pages =
                nodeJson[pagesName].get<std::unordered_set<int>>();
            const size_t jsonNodeId = nodeJson[nodeIdName].get<size_t>();

            std::vector<size_t> parents;
            for (const size_t jsonNodeParent : nodeJson[nodeParentsName].get<std::vector<int>>()) {
                parents.push_back(fromJsonNode.at(jsonNodeParent));
            }

            const size_t node = nodeJson.contains(guestPagesName)
                                    ? instance.addLeaf(parents, parseGuest(nodeJson), pages)
                                    : instance.addInner(cluster, parents, pages);

            fromJsonNode[jsonNodeId] = node;
        }
    }

    for (const auto &clusterChildJson : clusterJson[clusterChildrenName]) {
        parseClusterSubtree(instance, cluster, clusterChildJson, fromJsonNode, false);
    }
}

std::vector<ClusterTreeInstance> ClusterTreeInstanceParser::load(const int maxInstances)
{
    namespace fs = std::filesystem;

    std::vector<ClusterTreeInstance> instances;
    instances.reserve(maxInstances);

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

            const size_t capacity = instanceJson[capacityName].get<size_t>();
            std::unordered_map<size_t, size_t> jsonToNodeIds;

            ClusterTreeInstance instance(capacity);
            parseClusterSubtree(instance, ClusterTreeInstance::getRootCluster(), instanceJson,
                                jsonToNodeIds, true);

            instances.push_back(std::move(instance));
            ++processedInstances[path];

            if (instances.size() == maxInstances) {
                return instances;
            }
        }
    }

    return instances;
}

}  // namespace vmp
