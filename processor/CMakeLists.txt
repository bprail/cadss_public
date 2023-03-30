project(processor)
add_library(processor SHARED processor.c)
target_include_directories(processor PRIVATE ../common)
