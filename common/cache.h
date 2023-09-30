#ifndef CACHE_H
#define CACHE_H

#include "trace.h"
#include "coherence.h"
#include "common.h"

typedef struct _cache_sim_args {
    int arg_count;
    char** arg_list;
    coher* coherComp;
} cache_sim_args;

typedef struct _cache {
    sim_interface si;
    void (*memoryRequest)(trace_op*, int, int64_t,
                          void (*callback)(int, int64_t));
    debug_env_vars dbgEnv;
} cache;

#endif
