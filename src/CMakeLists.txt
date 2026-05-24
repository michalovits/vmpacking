target_sources(vmp PRIVATE
        vmp_clustertreeinstance.cpp
        vmp_generalinstance.cpp
        vmp_guest.cpp
        vmp_host.cpp
        vmp_maximisers.cpp
        vmp_packing.cpp
        vmp_solverutils.cpp
        vmp_treeinstance.cpp
)

target_include_directories(vmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(parsers)
