#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "trace.h"
#include "cache.h"
#include "branch.h"
#include "common.h"

typedef struct _processor_sim_args {
    trace_reader* tr;
    cache* cache_sim;
    branch* branch_sim;
    int arg_count;
    char** arg_list;
} processor_sim_args;

typedef struct _processor {
    sim_interface si;
    debug_env_vars dbgEnv;
} processor;

#endif
