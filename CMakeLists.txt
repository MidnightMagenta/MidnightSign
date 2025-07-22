cmake_minimum_required(VERSION 3.10)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ProjectName "MD-Sign")
find_library(MONOCYPHER_LIB monocypher)

add_executable("md-keygen" 
    #src
    src/md-keygen.cpp
    #headers
)

target_link_libraries(md-keygen PRIVATE ${MONOCYPHER_LIB})

add_executable("md-keytoarr"
    src/pk-arr.cpp
)