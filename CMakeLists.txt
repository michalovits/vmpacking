cmake_minimum_required(VERSION 3.14)

project(vmp CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address,undefined")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif ()

include(FetchContent)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

add_library(vmp STATIC)
target_compile_options(vmp PRIVATE -Wall -Wextra)
target_link_libraries(vmp PUBLIC nlohmann_json::nlohmann_json)

add_subdirectory(src)
add_subdirectory(examples)

option(VMP_BUILD_TESTS "Build vmp unit tests" ON)
if (VMP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()