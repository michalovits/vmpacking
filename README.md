# vmpacking (vmp)

Algorithms for the overlap variant of the Bin Packing problem, VM Packing. Built as part of my undergraduate dissertation at St Andrews in Spring 2025.

Direct approximations ([source code](src/vmp_solvers.h)):

* Next Fit
* First Fit
* Greedy Placement by Efficiency (implements [[1]](https://doi.org/10.1016/j.cie.2017.10.015))
* Overload and Remove (also implements [[1]](https://doi.org/10.1016/j.cie.2017.10.015))
* Greedy Placement by Opportunity-Aware Efficiency (specialises [[2]](https://doi.org/10.3390/electronics12204205) to the single-resource fixed-capacity case)
* Tree Placement (implements [[3]](https://doi.org/10.1145/1989493.1989554))

Approximations are also possible by reducing VM Packing to VM Maximisation and in turn to Single-Host VM Maximisation ([source code](src/vmp_maximisers.h)). The dissertation proves that this reduction chain preserves approximation bounds:

* VM Packing to VM Maximisation: If the maximiser is β-approximate (realises at least a β fraction of optimal reward, 0 < β < 1), the VM Packing approximation is within O(-lg |guests| / lg (1-β)) of optimal
* VM Maximisation to Single-Host VM Maximisation: If the single-host solver is α-approximate, the VM Maximisation approximated to within α/(α+1) - ε for any ε > 0 (uses [[4]](https://dl.acm.org/doi/10.5555/1109557.1109624))

Approximations via VM Maximisation:

* Greedy Placement by Subset Efficiency (specialises and unifies the strategies of [[5]](https://www.sciencedirect.com/science/article/abs/pii/S0167739X18302619) and [[6]](https://doi.org/10.1109/TSC.2017.2786728) to use as a single-host maximiser)
* Cluster-Tree Placement (uses [[3]](https://doi.org/10.1145/1989493.1989554) as a single-host maximiser)

Treatments (applied before or after solving):

* Tree-Ordering / Cluster-Tree-Ordering pre-treatments (reuse the data structures from [[3]](https://doi.org/10.1145/1989493.1989554))
* Decanting post-treatment (implements [[1]](https://doi.org/10.1016/j.cie.2017.10.015))

## Usage Examples

See the [examples](examples) directory.

## Building

Requirements:

* CMake 3.14 or newer
* C++ 20 with `std::ranges`

In the project root, create a build directory:

```shell
mkdir -p build && cd build
```

Configure the project (`Release` or `Debug`):

```shell
cmake .. -DCMAKE_BUILD_TYPE=Release
```

To skip building tests, configure with `-DVMP_BUILD_TESTS=OFF`.

To build the benchmarks binary, configure with `-DVMP_BUILD_BENCHMARKS=ON`:

```shell
cmake .. -DVMP_BUILD_BENCHMARKS=ON
```

Build the project:

```shell
cmake --build .
```

## Benchmarking

`vmp_benchmarks` runs every solver over each instance in three suite directories (`general`, `tree`, `cluster-tree`) and reports time measurements to stdout as CSV (tuples of `suite, instance, solver, guests, capacity, hosts, time_ms, valid`). Missing directories are skipped.

```shell
vmp_benchmarks /path/to/instances     # defaults to threads = logical cores
vmp_benchmarks /path/to/instances -w4 # 4 threads
vmp_benchmarks /path/to/instances -w1 # 1 thread (likely most representative measurements)
```

Instances are loaded and solved in batches of `min(32, workers * 4)` that are parallelised within.
The tradeoff is between effective load balancing and contention for shared caches.

You may want to experiment with this on your system!
Override with `-b N` / `--batch N`.

## Testing

Tests are built as two binaries:

* `vmp_unit_tests` ([tests/unit](tests/unit))
* `vmp_e2e_tests` ([tests/e2e/data](tests/e2e/data))

Build as above, and in the build directory:

```shell
ctest
```

Override the input directory for e2e at configure time with `-DVMP_TEST_INSTANCE_DIR=/path/to/instances`.
The tests expect the `general/`, `tree/` and `cluster-tree/` subdirectories and skip if they are missing.
