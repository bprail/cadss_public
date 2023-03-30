cmake_minimum_required(VERSION 3.0)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# For debugging.
# set(CMAKE_C_FLAGS "-O0 -ggdb3 -DDEBUG")

set(CMAKE_C_FLAGS "-O2 -ggdb -DDEBUG")

add_subdirectory(branch)
add_subdirectory(cache)
add_subdirectory(engine)
add_subdirectory(trace)
add_subdirectory(processor)
add_subdirectory(coherence)
add_subdirectory(interconnect)
add_subdirectory(simpleCache)
add_subdirectory(memory)

project(cadss C)
