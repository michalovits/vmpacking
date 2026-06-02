target_sources(vmp PRIVATE
        vmp_clustertree.cpp
        vmp_clustertreebuilder.cpp
        vmp_instance.cpp
        vmp_tree.cpp
        vmp_treebuilder.cpp
)

target_include_directories(vmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
