find_package(Threads REQUIRED)

add_executable(vmp_benchmarks
        benchmark.cpp
)

target_link_libraries(vmp_benchmarks PRIVATE vmp Threads::Threads)
