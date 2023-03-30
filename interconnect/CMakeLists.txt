project(interconnect)
add_library(interconnect SHARED interconnect.c)
target_include_directories(interconnect PRIVATE ../common)
