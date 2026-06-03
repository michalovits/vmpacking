#ifndef VMP_TREELOADER_H
#define VMP_TREELOADER_H

#include <vmp_treebuilder.h>

#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <vector>

namespace vmp
{

class TreeLoader
{
  public:
    explicit TreeLoader(std::string directory, std::string capacityName = "capacity",
                        std::string guestPagesName = "guest_pages", std::string pagesName = "pages",
                        std::string childrenName = "children");

    [[nodiscard]]
    std::vector<Tree> load(size_t maxInstances = std::numeric_limits<size_t>::max());

  private:
    const std::string directory;

    const std::string capacityName;
    const std::string guestPagesName;
    const std::string pagesName;
    const std::string childrenName;

    std::set<std::filesystem::path> paths;
    std::unordered_map<std::filesystem::path, size_t> processedInstances;

    [[nodiscard]] std::optional<Guest> parseGuest(const nlohmann::json &nodeJson) const;

    void parseChildren(TreeBuilder &builder, size_t parent, const nlohmann::json &nodeJson) const;
};

}  // namespace vmp

#endif  // VMP_TREELOADER_H
