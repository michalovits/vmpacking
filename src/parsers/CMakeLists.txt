target_sources(vmp PRIVATE
        vmp_clustertreeinstanceparser.cpp
        vmp_generalinstanceparser.cpp
        vmp_treeinstanceparser.cpp
)

target_include_directories(vmp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
