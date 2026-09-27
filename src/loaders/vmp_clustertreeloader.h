#ifndef VMP_CLUSTERTREELOADER_H
#define VMP_CLUSTERTREELOADER_H

#include <vmp_clustertreebuilder.h>

#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <vector>

namespace vmp
{

class ClusterTreeLoader
{
  public:
    explicit ClusterTreeLoader(std::string directory, std::string capacityName = "capacity",
                               std::string nodesName = "nodes", std::string nodeIdName = "node_id",
                               std::string nodeParentsName = "node_parents",
                               std::string pagesName = "node_pages",
                               std::string guestPagesName = "guest_pages",
                               std::string clusterChildrenName = "cluster_children");

    [[nodiscard]]
    std::vector<ClusterTree> load(size_t maxInstances = std::numeric_limits<size_t>::max());

  private:
    const std::string directory_;

    const std::string capacityName_;
    const std::string nodesName_;
    const std::string nodeIdName_;
    const std::string nodeParentsName_;
    const std::string pagesName_;
    const std::string guestPagesName_;
    const std::string clusterChildrenName_;

    std::set<std::filesystem::path> paths_;
    std::unordered_map<std::filesystem::path, size_t> processedInstances_;

    [[nodiscard]] std::optional<Guest> parseGuest(const nlohmann::json &nodeJson) const;

    void parseClusterSubtree(ClusterTreeBuilder &builder, size_t parentCluster,
                             const nlohmann::json &clusterJson,
                             std::unordered_map<size_t, size_t> &fromJsonNode,
                             bool skipRoot = false) const;
};

};  // namespace vmp

#endif  // VMP_TREELOADER_H
