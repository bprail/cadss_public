#include <getopt.h>
#include <stdio.h>

#include <memory.h>
#include "interconn.h"


interconn * self_c;

void busReq(bus_req_type brt, uint64_t addr, int procNum){
    busReq_cpp(brt, addr, procNum);
}


void registerCoher(coher* cc, void ** cohStateTree)
{
    registerCoher_cpp(cc, cohStateTree);
}

int tick()
{
    return tick_cpp();
}
// Return a non-zero value if the current request
// was satisfied by a cache-to-cache transfer.
int busReqCacheTransfer(uint64_t addr, int procNum)
{
    return busReqCacheTransfer_cpp(addr, procNum);
}

 int finish(int outFd)
{
    return finish_cpp(outFd);
}

 int destroy(void)
{
    return destroy_cpp();
}

interconn* init(inter_sim_args* isa)
{
    self_c = malloc(sizeof(interconn));
    self_c->busReq = busReq;
    self_c->registerCoher = registerCoher;
    self_c->busReqCacheTransfer = busReqCacheTransfer;
    self_c->si.tick = tick;
    self_c->si.finish = finish;
    self_c->si.destroy = destroy;
    
    init_cpp(isa,  self_c);
    return self_c;
}