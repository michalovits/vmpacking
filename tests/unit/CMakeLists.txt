add_executable(vmp_unit_tests
        test_guest.cpp
        test_host.cpp
        test_instance.cpp
        test_tree.cpp
        test_clustertree.cpp
        test_packing.cpp
        test_solverutils.cpp
)

target_link_libraries(vmp_unit_tests PRIVATE vmp Catch2::Catch2WithMain)
target_include_directories(vmp_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/helpers)

catch_discover_tests(vmp_unit_tests)
