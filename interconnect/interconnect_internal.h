#include <memory.h>
#include <interconnect.h>

#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
void init_cpp(inter_sim_args* isa, interconn* self_c);
int tick_cpp();
int finish_cpp(int outFd);
int destroy_cpp(void);

void registerCoher_cpp(coher* cc);
void busReq_cpp(bus_req_type brt, uint64_t addr, int procNum);
int busReqCacheTransfer_cpp(uint64_t addr, int procNum);

#ifdef __cplusplus  
}
#endif


//C functions for C++ to call

typedef enum _bus_req_state
{
    NONE,
    QUEUED,
    TRANSFERING_CACHE,
    TRANSFERING_MEMORY,
    WAITING_CACHE,
    WAITING_MEMORY
} bus_req_state;

typedef struct _bus_req {
    bus_req_type brt;
    bus_req_state currentState;
    uint64_t addr;
    int procNum;
    uint8_t shared;
    uint8_t data;
    uint8_t dataAvail;
    struct _bus_req* next;
} bus_req;