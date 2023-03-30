#include <cache.h>
#include <trace.h>

#include <getopt.h>
#include <stdlib.h>
#include <assert.h>

#include <coherence.h>
#include "stree.h"

cache* self = NULL;

int processorCount = 1;
int CADSS_VERBOSE = 0;
int blockSize = 1;

coher* coherComp = NULL;

int64_t* pendingTag = NULL;

typedef void (*memCallbackFunc)(int, int64_t);

memCallbackFunc* memCallback = NULL;

void coherCallback(int type, int processorNum, int64_t addr);
void memoryRequest(trace_op* op, int processorNum, int64_t tag,
                   void (*callback)(int, int64_t));

cache* init(cache_sim_args* csa)
{
    int op;

    // TODO - get argument list from assignment
    while ((op = getopt(csa->arg_count, csa->arg_list, "E:s:b:i:R:")) != -1)
    {
        switch (op)
        {
            // Lines per set
            case 'E':

                break;

            // Sets per cache
            case 's':

                break;

            // block size in bits
            case 'b':
                blockSize = 0x1 << atoi(optarg);
                break;

            // entries in victim cache
            case 'i':

                break;

            // bits in a RRIP-based replacement policy
            case 'R':

                break;
        }
    }

    self = malloc(sizeof(cache));
    self->memoryRequest = memoryRequest;
    self->si.tick = tick;
    self->si.finish = finish;
    self->si.destroy = destroy;

    coherComp = csa->coherComp;
    coherComp->registerCacheInterface(coherCallback);

    memCallback = calloc(processorCount, sizeof(memCallbackFunc));
    pendingTag = calloc(processorCount, sizeof(int));

    return self;
}

typedef struct _pendingRequest {
    int64_t tag;
    int64_t addr;
    int processorNum;
    void (*callback)(int, int64_t);
    struct _pendingRequest* next;
} pendingRequest;

pendingRequest* readyReq = NULL;
pendingRequest* pendReq = NULL;

// type could be READ, WRITE, INVALIDATE, simple ignores this
void coherCallback(int type, int processorNum, int64_t addr)
{
    assert(pendReq != NULL);
    assert(processorNum < processorCount);

    // "simpleCache" does not support invalidations.
    if (type != DATA_RECV)
        return;

    if (pendReq->processorNum == processorNum && pendReq->addr == addr)
    {
        pendingRequest* pr = pendReq;
        pendReq = pendReq->next;

        pr->next = readyReq;
        readyReq = pr;
    }
    else
    {
        pendingRequest* prevReq = pendReq;
        pendingRequest* pr = pendReq->next;

        while (pr != NULL)
        {
            if (pr->processorNum == processorNum && pr->addr == addr)
            {
                prevReq->next = pr->next;

                pr->next = readyReq;
                readyReq = pr;
                break;
            }
            pr = pr->next;
            prevReq = prevReq->next;
        }

        if (pr == NULL && CADSS_VERBOSE == 1)
        {
            pr = pendReq;
            while (pr != NULL)
            {
                printf("W: %p (%lx %d)\t", pr, pr->addr, pr->processorNum);
                pr = pr->next;
            }
        }
        assert(pr != NULL);
    }
}

void memoryRequest(trace_op* op, int processorNum, int64_t tag,
                   void (*callback)(int, int64_t))
{
    assert(op != NULL);
    assert(callback != NULL);

    // As a simplifying assumption, requests do not cross cache lines
    uint64_t addr = (op->memAddress & ~(blockSize - 1));
    uint8_t perm
        = coherComp->permReq((op->op == MEM_LOAD), addr, processorNum);

    pendingRequest* pr = malloc(sizeof(pendingRequest));
    pr->tag = tag;
    pr->addr = addr;
    pr->callback = callback;
    pr->processorNum = processorNum;

    if (perm == 1)
    {
        // create callback for next tick
        pr->next = readyReq;
        readyReq = pr;
    }
    else
    {
        // create pending callback
        pr->next = pendReq;
        pendReq = pr;
    }
}

int tick()
{
    coherComp->si.tick();

    pendingRequest* pr = readyReq;
    while (pr != NULL)
    {
        pendingRequest* t = pr;
        pr->callback(pr->processorNum, pr->tag);
        pr = pr->next;
        free(t);
    }
    readyReq = NULL;

    return 1;
}

int finish(int outFd)
{
    return 0;
}

int destroy(void)
{
    // free any internally allocated memory here
    return 0;
}
