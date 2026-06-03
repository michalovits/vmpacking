#include <catch2/catch_test_macros.hpp>

#include <vmp_clustertree.h>
#include <vmp_clustertreeloader.h>
#include <vmp_instance.h>
#include <vmp_instanceloader.h>
#include <vmp_packing.h>
#include <vmp_solvers.h>
#include <vmp_tree.h>
#include <vmp_treeloader.h>

#include <filesystem>

#ifndef VMP_TEST_INSTANCE_DIR
#error "VMP_TEST_INSTANCE_DIR is not specified"
#endif

namespace
{

template <typename Instance>
void requireValidPacking(vmp::Packing (*solve)(const Instance &), const Instance &instance)
{
    REQUIRE(solve(instance).validateForInstance(instance) == vmp::PACKING_OKAY);
}

std::string getDir(const char *subdir)
{
    return std::string(VMP_TEST_INSTANCE_DIR) + "/" + subdir;
}

using SetGuestIt = std::unordered_set<const vmp::Guest *>::const_iterator;

#define SKIP_IF_MISSING(dir)                              \
    do {                                                  \
        if (!std::filesystem::is_directory(dir)) {        \
            SKIP("input directory not found: " << (dir)); \
        }                                                 \
    } while (0)

}  // namespace

TEST_CASE("e2e: general instances", "[e2e]")
{
    const auto dir = getDir("general");
    SKIP_IF_MISSING(dir);

    auto loader = vmp::InstanceLoader(dir);
    const auto instances = loader.load();
    REQUIRE_FALSE(instances.empty());

    for (size_t i = 0; i < instances.size(); ++i) {
        INFO("general instance index: " << i);
        const auto &inst = instances[i];

        requireValidPacking(vmp::solveByNextFit, inst);
        requireValidPacking(vmp::solveByFirstFit, inst);
        requireValidPacking(vmp::solveByEfficiency, inst);
        requireValidPacking(vmp::solveByOpportunityAwareEfficiency, inst);
        requireValidPacking(vmp::solveByOverloadAndRemove, inst);
    }
}

TEST_CASE("e2e: tree instances", "[e2e]")
{
    const auto dir = getDir("tree");
    SKIP_IF_MISSING(dir);

    auto loader = vmp::TreeLoader(dir);
    const auto instances = loader.load();
    REQUIRE_FALSE(instances.empty());

    const auto solveByTreeFirstFit = [](const vmp::Tree &inst) {
        return vmp::solveByTree<SetGuestIt>(inst, vmp::proceedByFirstFit);
    };
    const auto solveByTreeEfficiency = [](const vmp::Tree &inst) {
        return vmp::solveByTree<SetGuestIt>(inst, vmp::proceedByEfficiency);
    };
    const auto solveByTreeOverloadAndRemove = [](const vmp::Tree &inst) {
        return vmp::solveByTree<SetGuestIt>(inst, vmp::proceedByOverloadAndRemove);
    };

    for (size_t i = 0; i < instances.size(); ++i) {
        INFO("tree instance index: " << i);
        const auto &inst = instances[i];

        requireValidPacking(+solveByTreeFirstFit, inst);
        requireValidPacking(+solveByTreeEfficiency, inst);
        requireValidPacking(+solveByTreeOverloadAndRemove, inst);
        requireValidPacking(vmp::solveByOpportunityAwareEfficiency, inst);
        requireValidPacking(vmp::solveByOverloadAndRemove, inst);
    }
}

TEST_CASE("e2e: cluster-tree instances", "[e2e]")
{
    const auto dir = getDir("cluster-tree");
    SKIP_IF_MISSING(dir);

    auto loader = vmp::ClusterTreeLoader(dir);
    const auto instances = loader.load();
    REQUIRE_FALSE(instances.empty());

    for (size_t i = 0; i < instances.size(); ++i) {
        INFO("cluster-tree instance index: " << i);
        const auto &inst = instances[i];

        requireValidPacking(vmp::solveByNextFit, inst);
        requireValidPacking(vmp::solveByFirstFit, inst);
        requireValidPacking(vmp::solveByEfficiency, inst);
        requireValidPacking(vmp::solveByOpportunityAwareEfficiency, inst);
        requireValidPacking(vmp::solveByOverloadAndRemove, inst);
    }
}
