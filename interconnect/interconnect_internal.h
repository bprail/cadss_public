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

void registerCoher_cpp(coher* cc, void ** cohStateTree);
void busReq_cpp(bus_req_type brt, uint64_t addr, int procNum);
int busReqCacheTransfer_cpp(uint64_t addr, int procNum);

#ifdef __cplusplus  
}
#endif

//C functions for C++ to call