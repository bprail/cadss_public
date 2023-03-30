project(trace)

add_library(trace SHARED trace.c)
target_include_directories(trace PRIVATE ../common)

add_subdirectory(taskLib)