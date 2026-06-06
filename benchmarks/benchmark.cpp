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
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace
{

struct Config
{
    std::string generalDir;
    std::string treeDir;
    std::string clusterTreeDir;

    unsigned workers;
    size_t maxResidentInstances;  // sets the number of instances that are loaded at once

    bool help;
};

struct Result
{
    std::string suiteLabel;
    std::string solverLabel;
    std::string instanceLabel;

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

    static std::string csvHeader()
    {
        return "suite,instance,solver,guests,capacity,hosts,time_ms,valid";
    }

    [[nodiscard]] std::string csvRow() const
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
    std::cout << "Usage: " << program << " <instance-dir> [-w N | --workers N] [-b N | --batch N]\n"
              << "  Runs every solver over each instance in the three benchmark\n"
              << "  suites under <instance-dir> and reports timings to stdout as CSV.\n\n"
              << "  -w N, --workers N  number of worker threads (default: logical cores)\n"
              << "  -b N, --batch N    instances loaded per batch (default: min(32, workers*4))\n";
}

Config parseConfig(int argc, char **argv)
{
    auto cfg = Config{ .workers = std::max(1u, std::thread::hardware_concurrency()),
                       .maxResidentInstances = 0,
                       .help = false };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            cfg.help = true;
            continue;
        }

        if (arg[0] != '-') {
            if (cfg.generalDir.empty()) {
                cfg.generalDir = arg + "/general";
                cfg.treeDir = arg + "/tree";
                cfg.clusterTreeDir = arg + "/cluster-tree";
            }
            continue;
        }

        std::string flag;
        std::string value;
        if (arg == "-w" || arg == "--workers") {
            flag = "w";
            if (i + 1 < argc) {
                value = argv[++i];
            }
        }
        else if (arg.rfind("-w", 0) == 0) {
            flag = "w";
            value = arg.substr(2);
        }
        else if (arg.rfind("--workers=", 0) == 0) {
            flag = "w";
            value = arg.substr(10);
        }
        else if (arg == "-b" || arg == "--batch") {
            flag = "b";
            if (i + 1 < argc) {
                value = argv[++i];
            }
        }
        else if (arg.rfind("-b", 0) == 0) {
            flag = "b";
            value = arg.substr(2);
        }
        else if (arg.rfind("--batch=", 0) == 0) {
            flag = "b";
            value = arg.substr(8);
        }

        if (!value.empty()) {
            const int parsed = std::atoi(value.c_str());
            if (parsed > 0) {
                if (flag == "w") {
                    cfg.workers = static_cast<unsigned>(parsed);
                }
                if (flag == "b") {
                    cfg.maxResidentInstances = static_cast<size_t>(parsed);
                }
            }
        }
    }

    if (cfg.maxResidentInstances == 0) {  // default if still unset
        cfg.maxResidentInstances = std::min(32u, cfg.workers * 4);
    }

    return cfg;
}

template <typename LoaderT, typename InstanceT>
void runSuite(const char *suiteLabel, const std::vector<Solver<InstanceT>> &solvers,
              const std::string &dir, unsigned workers, size_t maxResidentInstances,
              std::mt19937 &rng, std::vector<Result> &outResults)
{
    if (!std::filesystem::is_directory(dir)) {
        std::cerr << "skipping suite '" << suiteLabel << "': directory not found (" << dir << ")\n";
        return;
    }

    LoaderT loader(dir);

    while (true) {
        auto instances = loader.load(maxResidentInstances);
        if (instances.empty()) {
            break;
        }

        auto tasks = makeTasks(suiteLabel, instances, solvers);
        std::shuffle(tasks.begin(), tasks.end(), rng);

        auto results = runTasks(tasks, workers);

        for (auto &res : results) {
            outResults.push_back(std::move(res));
        }
    }
}

auto decanted(auto solver)
{
    return [solver = std::move(solver)](const auto &instance) {
        auto packing = solver(instance);
        packing.decantGuests();
        return packing;
    };
}

std::vector<Solver<vmp::Instance>> generalSolvers()
{
    return {
        { "nf", decanted(vmp::solveByNextFit<vmp::Instance>) },
        { "ff", vmp::solveByFirstFit<vmp::Instance> },  // first-fit never needs decanting
        { "eff", decanted(vmp::solveByEfficiency<vmp::Instance>) },
        { "opp", decanted(vmp::solveByOpportunityAwareEfficiency<vmp::Instance>) },
        { "oload", decanted(vmp::solveByOverloadAndRemove<vmp::Instance>) },

        // Too slow, not worth it:

        // { "subs-2", decanted([](const vmp::Instance &i) {
        //       return vmp::solveByLocalSubsetEfficiency(i, 2);
        //   }) },
    };
}

std::vector<Solver<vmp::Tree>> treeSolvers()
{
    using SetGuestIt = std::unordered_set<const vmp::Guest *>::const_iterator;

    return {
        { "nf-on-tree", decanted(vmp::solveByNextFit<vmp::Tree>) },
        { "ff-on-tree", vmp::solveByFirstFit<vmp::Tree> },
        { "eff-on-tree", decanted(vmp::solveByEfficiency<vmp::Tree>) },
        { "opp-on-tree", decanted(vmp::solveByOpportunityAwareEfficiency<vmp::Tree>) },
        { "oload-on-tree", decanted(vmp::solveByOverloadAndRemove<vmp::Tree>) },
        { "tree-by-ff",
          [](const vmp::Tree &t) {  // first-fit never needs decanting
              return vmp::solveByTree<SetGuestIt>(t, vmp::proceedByFirstFit);
          } },
        { "tree-by-eff", decanted([](const vmp::Tree &t) {
              return vmp::solveByTree<SetGuestIt>(t, vmp::proceedByEfficiency);
          }) },
    };
}

std::vector<Solver<vmp::ClusterTree>> clusterTreeSolvers()
{
    return {
        { "nf-on-cluster", decanted(vmp::solveByNextFit<vmp::ClusterTree>) },
        { "ff-on-cluster", vmp::solveByFirstFit<vmp::ClusterTree> },
        { "eff-on-cluster", decanted(vmp::solveByEfficiency<vmp::ClusterTree>) },
        { "opp-on-cluster", decanted(vmp::solveByOpportunityAwareEfficiency<vmp::ClusterTree>) },
        { "oload-on-cluster", decanted(vmp::solveByOverloadAndRemove<vmp::ClusterTree>) },
        { "cluster",
          decanted([](const vmp::ClusterTree &t) { return vmp::solveByLocalClusterTree(t); }) },
    };
}

template <typename InstanceT>
Task makeTask(std::string suiteLabel, const InstanceT *instance, Solver<InstanceT> solver)
{
    return [instance, suiteLabel = std::move(suiteLabel), solver = std::move(solver)]() {
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
std::vector<Task> makeTasks(const std::string &suiteLabel, const std::vector<InstanceT> &instances,
                            const std::vector<Solver<InstanceT>> &solvers)
{
    std::vector<Task> tasks;
    for (const auto &instance : instances) {
        for (const auto &solver : solvers) {
            tasks.push_back(makeTask(suiteLabel, &instance, solver));
        }
    }
    return tasks;
}

std::vector<Result> runTasks(const std::vector<Task> &tasks, unsigned workers)
{
    const size_t total = tasks.size();
    std::cerr << "running " << total << " solve tasks(s) on " << workers << " thread(s)\n";

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
            std::cerr << "[" << done << "/" << total << "] " << res.csvRow() << std::endl;
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

    std::cerr << "completed " << total << " tasks(s) in " << std::fixed << std::setprecision(3)
              << elapsed << " s\n";

    return results;
}

}  // namespace

int main(int argc, char **argv)
{
    const Config cfg = parseConfig(argc, argv);

    if (cfg.help) {
        printUsage(argv[0]);
        return 0;
    }

    if (cfg.generalDir.empty() || cfg.treeDir.empty() || cfg.clusterTreeDir.empty()) {
        printUsage(argv[0]);
        return -1;
    }

    std::cerr << "config:" << " workers=" << cfg.workers
              << " maxResidentInstances=" << cfg.maxResidentInstances << " generalDir='"
              << cfg.generalDir << "'" << " treeDir='" << cfg.treeDir << "'"
              << " clusterTreeDir='" << cfg.clusterTreeDir << "'" << '\n';

    std::mt19937 rng(std::random_device{}());
    std::vector<Result> results;

    runSuite<vmp::InstanceLoader, vmp::Instance>("general", generalSolvers(), cfg.generalDir,
                                                 cfg.workers, cfg.maxResidentInstances, rng,
                                                 results);

    runSuite<vmp::TreeLoader, vmp::Tree>("tree", treeSolvers(), cfg.treeDir, cfg.workers,
                                         cfg.maxResidentInstances, rng, results);

    runSuite<vmp::ClusterTreeLoader, vmp::ClusterTree>("cluster-tree", clusterTreeSolvers(),
                                                       cfg.clusterTreeDir, cfg.workers,
                                                       cfg.maxResidentInstances, rng, results);

    int invalid = 0;
    std::cout << Result::csvHeader() << '\n';

    std::sort(results.begin(), results.end());

    for (const auto &res : results) {
        if (!res.valid) {
            ++invalid;
        }
        std::cout << res.csvRow() << '\n';
    }

    return invalid;
}
