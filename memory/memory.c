#include <stdlib.h>

#include <memory.h>
#include <interconnect.h>

// Describes a DRAM request.
typedef struct _memReq {
    int procNum;
    uint64_t addr;
    int squelch;
} memReq;

void registerInterconnect(interconn* interconnect);
int busReq(uint64_t addr, int procNum);
int dataAvail(uint64_t addr, int procNum);

memory* self = NULL;
memReq* pendingRequest = NULL;
interconn* interComp;
int countDown = 0;

// This is the same as "BUS_TIME" (for now).
const int DRAM_FETCH_TICKS = 90;

memory* init(memory_sim_args* args)
{
    // TODO: Add "getopt" when we have actual arguments.

    self = calloc(1, sizeof(memory));
    assert(self);

    self->registerInterconnect = registerInterconnect;
    self->busReq = busReq;
    self->dataAvail = dataAvail;
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

int busReq(uint64_t addr, int procNum)
{
    assert(pendingRequest == NULL);

    pendingRequest = calloc(1, sizeof(memReq));
    pendingRequest->addr = addr;
    pendingRequest->procNum = procNum;
    pendingRequest->squelch = 0;

    countDown = DRAM_FETCH_TICKS;

    return countDown;
}

// Returns a non-zero value if the data
// is available for a pending request.
int dataAvail(uint64_t addr, int procNum)
{
    int avail = 0, clear = 0;

    if (!pendingRequest)
        return avail;

    // No data to respond.
    if (pendingRequest->squelch)
    {
        clear = 1;
        goto done;
    }

    // If count-down has elapsed; respond with data.
    if (addr == pendingRequest->addr && procNum == pendingRequest->procNum)
        clear = avail = (countDown <= 0);

done:
    if (clear)
    {
        free(pendingRequest);
        pendingRequest = NULL;
    }

    return avail;
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
