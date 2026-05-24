set(VMP_TEST_INSTANCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data" CACHE PATH "e2e input data")

add_executable(vmp_e2e_tests
        test_e2e.cpp
)

target_link_libraries(vmp_e2e_tests PRIVATE vmp Catch2::Catch2WithMain)
target_compile_definitions(vmp_e2e_tests PRIVATE
        VMP_TEST_INSTANCE_DIR="${VMP_TEST_INSTANCE_DIR}"
)

catch_discover_tests(vmp_e2e_tests)
