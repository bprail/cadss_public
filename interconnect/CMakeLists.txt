project(interconnect)
add_library(interconnect SHARED interconnect.c interconnect.cpp)

set(CMAKE_CXX_FLAGS "-O2 -g -DNDEBUG -std=c++11 -fpermissive")
target_include_directories(interconnect PRIVATE ../common ./)
