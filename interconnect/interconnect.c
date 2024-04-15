#include <getopt.h>
#include <stdio.h>

#include <memory.h>
#include <interconnect_internal.h>


void busReq(bus_req_type brt, uint64_t addr, int procNum){
    if (pendingRequest == NULL)
    {
        assert(brt != SHARED);

        bus_req* nextReq = calloc(1, sizeof(bus_req));
        nextReq->brt = brt;
        nextReq->currentState = WAITING_CACHE;
        nextReq->addr = addr;
        nextReq->procNum = procNum;
        nextReq->dataAvail = 0;

        pendingRequest = nextReq;
        countDown = CACHE_DELAY;
        if (CADSS_VERBOSE && PENDING_REQUEST_DEBUG)
            fprintf(stdout, "new pendingRequest(proc=%d) from busReq\n", pendingRequest->procNum);


    } else if (brt == SHARED && pendingRequest->addr == addr) {
        pendingRequest->shared = 1;

    } else if (brt == DATA && pendingRequest->addr == addr) {   
        assert(pendingRequest->currentState == WAITING_MEMORY ||
               pendingRequest->currentState == TRANSFERING_CACHE ||
               pendingRequest->currentState == TRANSFERING_MEMORY);

        if (pendingRequest->currentState == WAITING_MEMORY) {
            // This DATA busReq addresses the current pending request
            pendingRequest->data = 1;
            pendingRequest->currentState = TRANSFERING_CACHE;
            countDown = CACHE_TRANSFER;
            return;
        } else if (CADSS_VERBOSE && PENDING_REQUEST_DEBUG) {
            // pendingRequest was already served by another proc's DATA busReq
            // Occurs because all sharing procs snoop a READ(X), and send data.
            // With correct directory/arbitration, this should not trigger
            fprintf(stdout, "Duplicate DATA busReq received from proc , ignoring.\n");
        }
    } else {
        assert(brt != SHARED);

        bus_req* nextReq = calloc(1, sizeof(bus_req));
        nextReq->brt = brt;
        nextReq->currentState = QUEUED;
        nextReq->addr = addr;
        nextReq->procNum = procNum;
        nextReq->dataAvail = 0;

        enqBusRequest(nextReq, procNum);
    }
}

interconn* init(inter_sim_args* isa)
{
    return init_cpp(isa);
}


void registerCoher(coher* cc)
{
    registerCoher_cpp(cc);
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