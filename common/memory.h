#ifndef MEMORY_H
#define MEMORY_H

struct _interconn;

#include "common.h"
#include "interconnect.h"

typedef struct _memory_sim_args {
    int arg_count;
    char** arg_list;
} memory_sim_args;

typedef struct _memory {
    sim_interface si;
    int (*busReq)(uint64_t addr, int procNum, void (*callback)(int, uint64_t));
    void (*registerInterconnect)(struct _interconn* interconnect);
    debug_env_vars dbgEnv;
} memory;

#endif /* !MEMORY_H */
