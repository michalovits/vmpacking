#ifndef VMP_CLUSTERTREEINSTANCELOADER_H
#define VMP_CLUSTERTREEINSTANCELOADER_H

#include <vmp_clustertreebuilder.h>

#include <limits>
#include <nlohmann/json.hpp>
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
    std::vector<ClusterTreeInstance> load(size_t maxInstances = std::numeric_limits<size_t>::max());

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
    std::unordered_map<std::filesystem::path, size_t> processedInstances;

    [[nodiscard]] std::shared_ptr<Guest> parseGuest(const nlohmann::json &nodeJson) const;

    void parseClusterSubtree(ClusterTreeBuilder &builder, size_t parentCluster,
                             const nlohmann::json &clusterJson,
                             std::unordered_map<size_t, size_t> &fromJsonNode,
                             bool skipRoot = false) const;
};

};  // namespace vmp

#endif  // VMP_TREEINSTANCELOADER_H
