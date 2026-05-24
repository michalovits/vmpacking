#include <catch2/catch_test_macros.hpp>

#include <vmp_clustertreeinstance.h>
#include <vmp_clustertreeinstanceparser.h>
#include <vmp_generalinstance.h>
#include <vmp_generalinstanceparser.h>
#include <vmp_packing.h>
#include <vmp_solvers.h>
#include <vmp_treeinstance.h>
#include <vmp_treeinstanceparser.h>

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

std::string inputDirectory(const char *subdir)
{
    return std::string(VMP_TEST_INSTANCE_DIR) + "/" + subdir;
}

using SetGuestIt = std::unordered_set<std::shared_ptr<const vmp::Guest>>::const_iterator;

#define SKIP_IF_MISSING(dir)                              \
    do {                                                  \
        if (!std::filesystem::is_directory(dir)) {        \
            SKIP("input directory not found: " << (dir)); \
        }                                                 \
    } while (0)

}  // namespace

TEST_CASE("e2e: general instances", "[e2e]")
{
    const auto dir = inputDirectory("general");
    SKIP_IF_MISSING(dir);

    auto parser = vmp::GeneralInstanceParser(dir);
    const auto instances = parser.load(1024);
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
    const auto dir = inputDirectory("tree");
    SKIP_IF_MISSING(dir);

    auto parser = vmp::TreeInstanceParser(dir);
    const auto instances = parser.load(1024);
    REQUIRE_FALSE(instances.empty());

    const auto solveByTreeFirstFit = [](const vmp::TreeInstance &inst) {
        return vmp::solveByTree<SetGuestIt>(inst, vmp::proceedByFirstFit);
    };
    const auto solveByTreeEfficiency = [](const vmp::TreeInstance &inst) {
        return vmp::solveByTree<SetGuestIt>(inst, vmp::proceedByEfficiency);
    };
    const auto solveByTreeOverloadAndRemove = [](const vmp::TreeInstance &inst) {
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
    const auto dir = inputDirectory("cluster-tree");
    SKIP_IF_MISSING(dir);

    auto parser = vmp::ClusterTreeInstanceParser(dir);
    const auto instances = parser.load(1024);
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
