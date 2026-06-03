target_sources(vmp PRIVATE
        vmp_clustertreeloader.cpp
        vmp_instanceloader.cpp
        vmp_treeloader.cpp
)

target_include_directories(vmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
