target_sources(vmp PRIVATE
        vmp_clustertreeparser.cpp
        vmp_instanceparser.cpp
        vmp_treeparser.cpp
)

target_include_directories(vmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
