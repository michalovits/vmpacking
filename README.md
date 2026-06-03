# vmpacking (vmp)

Algorithms for the overlap variant of the Bin Packing problem, VM Packing. Built as part of my undergraduate dissertation at St Andrews (spring 2025).

Approximations currently [implemented](src/vmp_solvers.h):

* Next Fit
* First Fit
* Greedy Placement by Efficiency
* Greedy Placement by Opportunity-Aware Efficiency
* Tree-Based Placement
* Overload and Remove
* Greedy Placement by Subset Efficiency (Reduction from VM Maximisation)
* Cluster Tree-Based Placement (Reduction from VM Maximisation)

Static treatments (applied before or after solving):

* Tree-Ordering pre-treatment
* Cluster-Tree-Ordering pre-treatment
* Decanting post-treatment

Several of these algorithms implement, reuse or take inspiration from other research, credited in the relevant code.

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

`vmp_benchmarks` runs every solver over each instance in the three suites (`general/`, `tree/`, `cluster-tree/`) and reports time measurements to stdout as CSV: `suite,instance,solver,guests,capacity,hosts,time_ms,valid`.
Missing suite directories are skipped.

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
