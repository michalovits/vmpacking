# vmpacking (vmp)

This library implements algorithms for the VM Packing problem as part of my St Andrews Undergraduate Dissertation.
Even though the dissertation has been submitted as of 4 April 2025, this repository is under development.
I hope to continue providing implementations of more algorithms as they come up.

Approximations currently ![implemented](src/vmp_solvers.h):

* Next Fit
* First Fit
* Greedy Placement by Efficiency
* Greedy Placement by Opportunity-Aware Efficiency
* Tree-Based Placement
* Overload and Remove
* Greedy Placement by Subset Efficiency (Reduction from VM Maximisation)
* Cluster Tree-Based Placement (Reduction from VM Maximisation)

All as credited in the relevant code.

Static treatments:

* Tree-/Cluster Tree-Ordering pre-treatment
* Decanting post-treatment

## Build Instructions

Before building, please make sure your toolchain supports the following:

* CMake 3.14 or newer
* C++ 20 support, including `std::ranges`

Begin by cloning the repository:

```shell
git clone https://github.com/smich42/vmpacking.git
cd vmpacking
```

Create a build directory:

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