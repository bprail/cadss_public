project(interconnect)
add_library(interconnect SHARED stree_cpp.c interconnect.c interconn.cpp)

set(CMAKE_CXX_FLAGS "-g -fpermissive")
target_include_directories(interconnect PRIVATE ../common)
