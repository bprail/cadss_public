project(simpleCache)
add_library(simpleCache SHARED cache.c stree.c)
target_include_directories(simpleCache PRIVATE ../common)
