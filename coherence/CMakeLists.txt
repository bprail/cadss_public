project(coherence)
add_library(coherence SHARED coherence.c protocol.c stree.c)
target_include_directories(coherence PRIVATE ../common)
