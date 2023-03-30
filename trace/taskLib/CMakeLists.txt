project(taskLib)

set(CMAKE_CXX_FLAGS "-O2 -g -DNDEBUG -std=c++11")

add_library(taskLib SHARED TaskGraphAPI.cpp TaskGraph.cpp TaskGraphInfo.cpp Task.cpp Backend.cpp Action.cpp ct_file.c)
target_link_libraries(taskLib z)

target_include_directories(taskLib PRIVATE ../../common)
