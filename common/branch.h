#ifndef BRANCH_H
#define BRANCH_H

#include "common.h"
#include "trace.h"

enum BRANCH_MODEL_TYPE
{
    DEFAULT = 0,
    GSHARE = 1,
    GSELECT = 2,
    YEH_PATT = 3,
};

typedef struct _branch_sim_args {
    int arg_count;
    char** arg_list;
} branch_sim_args;

typedef struct _branch {
    sim_interface si;
    uint64_t (*branchRequest)(trace_op*, int);
    debug_env_vars dbgEnv;
} branch;

#endif
