project(cache)
add_library(cache SHARED cache.c)
target_include_directories(cache PRIVATE ../common)
