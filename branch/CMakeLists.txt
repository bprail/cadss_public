project(branch)
add_library(branch SHARED branch.c)
target_include_directories(branch PRIVATE ../common)
