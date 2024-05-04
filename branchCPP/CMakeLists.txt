project(branchCPP)

add_library(branchCPP SHARED branch.c br.cpp)
target_include_directories(branchCPP PRIVATE ../common)
