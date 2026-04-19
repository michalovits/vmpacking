#ifndef VMP_CLUSTERTREEINSTANCELOADER_H
#define VMP_CLUSTERTREEINSTANCELOADER_H

#include <vmp_clustertreeinstance.h>

#include <json.hpp>
#include <set>
#include <vector>

namespace vmp
{

class ClusterTreeInstanceParser
{
  public:
    explicit ClusterTreeInstanceParser(std::string directory, std::string capacityName = "capacity",
                                       std::string nodesName = "nodes",
                                       std::string nodeIdName = "node_id",
                                       std::string nodeParentsName = "node_parents",
                                       std::string pagesName = "node_pages",
                                       std::string guestPagesName = "guest_pages",
                                       std::string clusterChildrenName = "cluster_children");

    [[nodiscard]]
    std::vector<ClusterTreeInstance> load(int maxInstances = -1);

    ~ClusterTreeInstanceParser() = default;

  private:
    const std::string directory;

    const std::string capacityName;
    const std::string nodesName;
    const std::string nodeIdName;
    const std::string nodeParentsName;
    const std::string pagesName;
    const std::string guestPagesName;
    const std::string clusterChildrenName;

    std::set<std::filesystem::path> paths;
    std::unordered_map<std::filesystem::path, int> processedInstances;

    [[nodiscard]] std::shared_ptr<Guest> parseGuest(const nlohmann::json &nodeJson) const;
    void parseClusterSubtree(ClusterTreeInstance &instance, size_t parentCluster,
                             const nlohmann::json &clusterJson,
                             std::unordered_map<size_t, size_t> &fromJsonNode,
                             bool skipRoot = false) const;
};

};  // namespace vmp

#endif  // VMP_TREEINSTANCELOADER_H
