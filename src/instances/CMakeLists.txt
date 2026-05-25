target_sources(vmp PRIVATE
        vmp_generaltopology.cpp
        vmp_clustertreetopology.cpp
        vmp_clustertreebuilder.cpp
        vmp_treetopology.cpp
        vmp_treebuilder.cpp
)

target_include_directories(vmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
