#include <cache.h>
#include <trace.h>

#include <getopt.h>
#include <stdlib.h>
#include <assert.h>

typedef struct _pendingRequest {
    int64_t tag;
    int8_t procNum;
    void (*memCallback)(int, int64_t);
} pendingRequest;

cache* self = NULL;
int processorCount = 1;
int CADSS_VERBOSE = 0;
pendingRequest pending = {0};
int countDown = 0;

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

    return self;
}

void memoryRequest(trace_op* op, int processorNum, int64_t tag,
                   void (*callback)(int, int64_t))
{
    assert(op != NULL);
    assert(callback != NULL);

    // Simple model to only have one outstanding memory operation
    if (countDown != 0)
    {
        assert(pending.memCallback != NULL);
        pending.memCallback(pending.procNum, pending.tag);
    }

    pending = (pendingRequest){
        .tag = tag, .procNum = processorNum, .memCallback = callback};

    // In a real cache simulator, the delay is based
    // on whether the request is a hit or miss.
    countDown = 1;
}

int tick()
{
    if (countDown > 0)
    {
        countDown--;
        if (countDown == 0)
        {
            assert(pending.memCallback != NULL);
            pending.memCallback(pending.procNum, pending.tag);
        }
    }

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
