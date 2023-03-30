project(memory)
add_library(memory SHARED memory.c)
target_include_directories(memory PRIVATE ../common)
