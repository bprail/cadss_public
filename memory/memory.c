#include <stdlib.h>

#include <memory.h>
#include <interconnect.h>

#include "memory_internal.h"

void registerInterconnect(interconn* interconnect);
int busReq(uint64_t addr, int procNum, void (*callback)(int, uint64_t));

memory* self = NULL;
memReq* pendingRequest = NULL;
interconn* interComp;
int countDown = 0;

// This is the same as "BUS_TIME".
const int DRAM_FETCH_TICKS = 90;

memory* init(memory_sim_args* args)
{
    // TODO: Add "getopt" when we have actual arguments.

    self = calloc(1, sizeof(memory));
    assert(self);

    self->registerInterconnect = registerInterconnect;
    self->busReq = busReq;
    self->si.tick = tick;
    self->si.finish = finish;
    self->si.destroy = destroy;
    pendingRequest = NULL;

    return self;
}

void registerInterconnect(interconn* interconnect)
{
    assert(self);
    assert(interconnect);

    interComp = interconnect;
}

int busReq(uint64_t addr, int procNum, void (*callback)(int, uint64_t))
{
    assert(pendingRequest == NULL);

    pendingRequest = calloc(1, sizeof(memReq));
    pendingRequest->addr = addr;
    pendingRequest->procNum = procNum;
    pendingRequest->squelch = 0;
    pendingRequest->callback = callback;

    countDown = DRAM_FETCH_TICKS;

    return countDown;
}

int tick()
{
    if (countDown > 0)
    {
        assert(pendingRequest);

        // Check if one of the caches responded to the request that we are
        // processing. If that's the case, we "squelch" the response and
        // make ourselves available for the next request.
        if (interComp->busReqCacheTransfer(pendingRequest->addr,
                                           pendingRequest->procNum))
        {
            pendingRequest->squelch = 1;
            countDown = 0;
            goto done;
        }

        countDown--;
    }

done:
    if (pendingRequest && countDown == 0)
    {
        if (!pendingRequest->squelch)
        {
            pendingRequest->callback(pendingRequest->procNum,
                                     pendingRequest->addr);
        }

        free(pendingRequest);
        pendingRequest = NULL;
    }

    return countDown;
}

int finish(int outFd)
{
    return 0;
}

int destroy(void)
{
    free(self);
    free(pendingRequest);

    return 0;
}
