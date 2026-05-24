add_executable(vmp_unit_tests
        test_guest.cpp
        test_host.cpp
        test_generalinstance.cpp
        test_treeinstance.cpp
        test_clustertreeinstance.cpp
        test_packing.cpp
)

target_link_libraries(vmp_unit_tests PRIVATE vmp Catch2::Catch2WithMain)
target_include_directories(vmp_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/helpers)

catch_discover_tests(vmp_unit_tests)
