#ifndef COHERENCE_H
#define COHERENCE_H

#include "common.h"
#include "interconnect.h"

#define READ_PERM 0
#define WRITE_PERM 1

struct _interconn;

typedef struct _coher_sim_args {
    int arg_count;
    char** arg_list;
    struct _interconn* inter;
} coher_sim_args;

typedef enum _cache_action
{
    NO_ACTION,
    DATA_RECV,
    INVALIDATE
} cache_action;

typedef struct _coher {
    sim_interface si;
    void (*registerCacheInterface)(void (*callback)(int, int, int64_t));
    uint8_t (*permReq)(uint8_t is_read, uint64_t addr, int processorNum);
    uint8_t (*invlReq)(uint64_t addr, int processorNum);
    uint8_t (*busReq)(bus_req_type reqType, uint64_t addr, int processorNum);
    debug_env_vars dbgEnv;
} coher;

typedef enum _coherence_states
{
    UNDEF = 0, // As tree find returns NULL, we need an unused for NULL
    MODIFIED,
    SHARED_STATE,
    INVALID,
    SHARED_MODIFIED,  // Shared and awaiting excl. M access
    INVALID_MODIFIED, // Inval and awaiting excl. M access
    INVALID_SHARED    // Invalid and awaiting shared data
} coherence_states;

#endif
