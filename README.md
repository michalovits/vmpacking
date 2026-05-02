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

## Building

Requirements:

* CMake 3.14 or newer
* C++ 20 with `std::ranges`

From the project root, create a build directory:

```shell
mkdir -p build && cd build
```

Configure the project (`Release` or `Debug`):

```shell
cmake .. -DCMAKE_BUILD_TYPE=Release
```

Build the project:

```shell
cmake --build .
```

## Usage Examples

See [examples](examples).
