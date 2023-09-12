project(cadss-engine)

add_executable(cadss-engine engine.c config.c debug.c)
target_link_libraries(cadss-engine dl)
target_include_directories(cadss-engine PRIVATE ../common)
