#ifndef INTERCONNECT_H
#define INTERCONNECT_H

struct _coher;
struct _memory;

typedef enum _dir_req_type
{
    NO_REQ,
    READSHARED,
    READEX,
    SNOOPINV,
    SNOOPDNGRADE,
    MEMORY_WB,
    MEMORY_RD
} dir_req_type; //used to be bus_req_type

#include "coherence.h"
#include "memory.h"
#include "common.h"

typedef struct _inter_sim_args {
    int arg_count;
    char** arg_list;
    struct _memory* memory;
} inter_sim_args;

typedef struct _interconn {
    sim_interface si;
    void (*busReq)(bus_req_type brt, uint64_t addr, int procNum);
    void (*registerCoher)(struct _coher* coherComp);
    int (*busReqCacheTransfer)(uint64_t addr, int procNum);
    debug_env_vars dbgEnv;
} interconn;

#endif
