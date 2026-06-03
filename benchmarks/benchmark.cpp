#include <vmp_clustertree.h>
#include <vmp_clustertreeloader.h>
#include <vmp_instance.h>
#include <vmp_instanceloader.h>
#include <vmp_packing.h>
#include <vmp_solvers.h>
#include <vmp_tree.h>
#include <vmp_treeloader.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace
{

using SetGuestIt = std::unordered_set<const vmp::Guest *>::const_iterator;

struct Config
{
    std::string generalDir;
    std::string treeDir;
    std::string clusterTreeDir;

    unsigned workers;
    bool help;
};

struct Result
{
    std::string suiteLabel;
    std::string instanceLabel;
    std::string solverLabel;

    size_t capacity;
    size_t guestCount;
    size_t hostCount;

    bool valid;
    double timeMs;

    bool operator<(const Result &other) const
    {
        return std::tie(suiteLabel, instanceLabel, solverLabel) <
               std::tie(other.suiteLabel, other.instanceLabel, other.solverLabel);
    }

    static std::string header()
    {
        return "suite,instance,solver,guests,capacity,hosts,time_ms,valid";
    }

    [[nodiscard]] std::string asRow() const
    {
        std::ostringstream row;
        row << suiteLabel << ',' << instanceLabel << ',' << solverLabel << ',' << guestCount << ','
            << capacity << ',' << hostCount << ',' << std::fixed << std::setprecision(3) << timeMs
            << ',' << (valid ? "true" : "false");
        return row.str();
    }
};

template <typename InstanceT>
struct Solver
{
    std::string label;
    std::function<vmp::Packing(const InstanceT &)> solve;
};

using Task = std::function<Result()>;

void printUsage(const char *program)
{
    std::cout << "Usage: " << program << " <instance-dir> [-w N | --workers N]\n"
              << "  Runs every solver over each instance in the three benchmark\n"
              << "  suites under <instance-dir> and reports timings as CSV on stdout.\n\n"
              << "  -w N, --workers N  worker threads (default: logical cores; -w1 for\n"
              << "                     single-threaded, most accurate timings)\n";
}

Config parseConfig(int argc, char **argv)
{
    Config config;
    config.workers = std::max(1u, std::thread::hardware_concurrency());
    config.help = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            config.help = true;
            continue;
        }

        std::string value;
        if (arg == "-w" || arg == "--workers") {
            if (i + 1 < argc) {
                value = argv[++i];
            }
        }
        else if (arg.rfind("-w", 0) == 0) {
            value = arg.substr(std::string("-w").size());
        }
        else if (arg.rfind("--workers=", 0) == 0) {
            value = arg.substr(std::string("--workers=").size());
        }
        else if (arg[0] != '-') {
            if (config.generalDir.empty()) {
                config.generalDir = arg + "/general";
            }
            if (config.treeDir.empty()) {
                config.treeDir = arg + "/tree";
            }
            if (config.clusterTreeDir.empty()) {
                config.clusterTreeDir = arg + "/cluster-tree";
            }
            continue;
        }
        else {
            continue;
        }

        const int parsed = std::atoi(value.c_str());
        if (parsed > 0) {
            config.workers = static_cast<unsigned>(parsed);
        }
    }

    return config;
}

template <typename LoaderT>
auto loadSuite(const char *suite, const std::string &dir)
{
    LoaderT loader(dir);
    using Instances = decltype(loader.load());

    if (!std::filesystem::is_directory(dir)) {
        std::cerr << "skipping suite '" << suite << "': directory not found (" << dir << ")\n";
        return Instances{};
    }

    auto instances = loader.load();
    std::cerr << "loaded suite '" << suite << "': " << instances.size() << " instance(s)\n";

    return instances;
}

std::vector<Solver<vmp::Instance>> generalSolvers()
{
    return {
        { "next-fit", vmp::solveByNextFit<vmp::Instance> },
        { "first-fit", vmp::solveByFirstFit<vmp::Instance> },
        { "efficiency", vmp::solveByEfficiency<vmp::Instance> },
        { "opportunity-efficiency", vmp::solveByOpportunityAwareEfficiency<vmp::Instance> },
        { "overload-remove", vmp::solveByOverloadAndRemove<vmp::Instance> },
    };
}

std::vector<Solver<vmp::Tree>> treeSolvers()
{
    const auto firstFit = [](const vmp::Tree &tree) {
        return vmp::solveByTree<SetGuestIt>(tree, vmp::proceedByFirstFit);
    };
    const auto efficiency = [](const vmp::Tree &tree) {
        return vmp::solveByTree<SetGuestIt>(tree, vmp::proceedByEfficiency);
    };
    const auto overloadAndRemove = [](const vmp::Tree &tree) {
        return vmp::solveByTree<SetGuestIt>(tree, vmp::proceedByOverloadAndRemove);
    };

    return {
        { "tree-first-fit", firstFit },
        { "tree-efficiency", efficiency },
        { "tree-overload-remove", overloadAndRemove },
        { "opportunity-efficiency", vmp::solveByOpportunityAwareEfficiency<vmp::Tree> },
        { "overload-remove", vmp::solveByOverloadAndRemove<vmp::Tree> },
    };
}

std::vector<Solver<vmp::ClusterTree>> clusterTreeSolvers()
{
    return {
        { "next-fit", vmp::solveByNextFit<vmp::ClusterTree> },
        { "first-fit", vmp::solveByFirstFit<vmp::ClusterTree> },
        { "efficiency", vmp::solveByEfficiency<vmp::ClusterTree> },
        { "opportunity-efficiency", vmp::solveByOpportunityAwareEfficiency<vmp::ClusterTree> },
        { "overload-remove", vmp::solveByOverloadAndRemove<vmp::ClusterTree> },
    };
}

template <typename InstanceT>
Task makeTask(std::string suiteLabel, const InstanceT *instance, Solver<InstanceT> solver)
{
    return [suiteLabel = std::move(suiteLabel), instance, solver = std::move(solver)]() {
        Result res = {
            .suiteLabel = suiteLabel,
            .solverLabel = solver.label,
            .instanceLabel = instance->label(),
            .capacity = instance->capacity(),
            .guestCount = instance->guestCount(),
        };

        const auto start = std::chrono::steady_clock::now();
        const vmp::Packing packing = solver.solve(*instance);
        const auto end = std::chrono::steady_clock::now();

        res.timeMs = std::chrono::duration<double, std::milli>(end - start).count();
        res.valid = packing.validateForInstance(*instance) == vmp::PACKING_OKAY;
        res.hostCount = packing.hostCount();

        return res;
    };
}

template <typename InstanceT>
void makeTasks(const std::string &suite, const std::vector<InstanceT> &instances,
               const std::vector<Solver<InstanceT>> &solvers, std::vector<Task> &tasks)
{
    for (const auto &instance : instances) {
        for (const auto &solver : solvers) {
            tasks.push_back(makeTask(suite, &instance, solver));
        }
    }
}

std::vector<Result> runTasks(const std::vector<Task> &tasks, unsigned workers)
{
    const size_t total = tasks.size();
    std::cerr << "running " << total << " task(s) on " << workers << " thread(s)\n";

    std::vector<Result> results(total);
    std::atomic<size_t> nextIdx = 0;
    std::atomic<size_t> completed = 0;
    std::mutex mutex;

    const auto work = [&]() {
        for (size_t i = nextIdx.fetch_add(1); i < total; i = nextIdx.fetch_add(1)) {
            results[i] = tasks[i]();

            const size_t done = completed.fetch_add(1) + 1;
            const Result &res = results[i];

            std::lock_guard<std::mutex> lock(mutex);
            std::cerr << "[" << done << "/" << total << "] " << res.asRow() << std::endl;
        }
    };

    const auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> pool;
    pool.reserve(workers);

    for (unsigned worker = 0; worker < workers; ++worker) {
        pool.emplace_back(work);
    }
    for (auto &thread : pool) {
        thread.join();
    }

    const auto end = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(end - start).count();

    std::cerr << "done: " << total << " task(s) in " << std::fixed << std::setprecision(3)
              << elapsed << " s\n";

    return results;
}

int countInvalid(const std::vector<Result> &results)
{
    return std::ranges::count_if(results, [](const Result &res) { return !res.valid; });
}

}  // namespace

int main(int argc, char **argv)
{
    const Config config = parseConfig(argc, argv);

    if (config.help) {
        printUsage(argv[0]);
        return 0;
    }

    if (config.generalDir.empty() || config.treeDir.empty() || config.clusterTreeDir.empty()) {
        printUsage(argv[0]);
        return -1;
    }

    // Load instances

    auto generals = loadSuite<vmp::InstanceLoader>("general", config.generalDir);
    auto trees = loadSuite<vmp::TreeLoader>("tree", config.treeDir);
    auto clusterTrees = loadSuite<vmp::ClusterTreeLoader>("cluster-tree", config.clusterTreeDir);

    if (generals.empty() && trees.empty() && clusterTrees.empty()) {
        return -1;
    }

    // Generate tasks

    std::vector<Task> tasks;
    makeTasks("general", generals, generalSolvers(), tasks);
    makeTasks("tree", trees, treeSolvers(), tasks);
    makeTasks("cluster-tree", clusterTrees, clusterTreeSolvers(), tasks);

    std::vector<Result> results = runTasks(tasks, config.workers);

    // stdout gets CSV output
    std::sort(results.begin(), results.end());

    std::cout << Result::header() << '\n';
    for (const Result &res : results) {
        std::cout << res.asRow() << '\n';
    }

    return countInvalid(results);
}
