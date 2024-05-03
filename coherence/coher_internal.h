#ifndef COHER_INTERNAL_H
#define COHER_INTERNAL_H

#include <interconnect.h>
#include <stdio.h>

extern interconn* inter_sim;

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

static const char* coherence_state_map[] = {
    [UNDEF] = "UNDEFINED",
    [MODIFIED] = "MODIFIED",
    [SHARED_STATE] = "SHARED",
    [INVALID] = "INVALID",
    [SHARED_MODIFIED] = "SHARED_MODIFIED",
    [INVALID_MODIFIED] = "INVALID_MODIFIED",
    [INVALID_SHARED] = "INVALID_SHARED"
};

typedef enum _coherence_scheme
{
    MI,
    MSI,
    MESI,
    MOESI,
    MESIF
} coherence_scheme;

coherence_states
cacheMI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum);
coherence_states
snoopMI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum);

coherence_states
cacheMSI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum);
coherence_states
snoopMSI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum);

#endif
