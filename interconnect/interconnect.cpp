#include <getopt.h>
#include <stdio.h>
#include <vector>
#include <memory.h>
#include <interconnect_internal.h>
#include <iostream>
#include <stree.h>
#include <coherence.h>
#include <assert.h>
#include <fstream>
#include <cstdio>
#include <sstream>

typedef std::vector<coherence_states> snoop_recipients;


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


bus_req* pendingRequest = NULL;
bus_req** queuedRequests;
tree_t ** coherStates;
interconn* self;
coher* coherComp;
memory* memComp;

int CADSS_VERBOSE = 0;
int processorCount = 1;

std::vector<long int> numSnoops;
// numSnoops.resize(processorCount, 0);

static const char* req_state_map[] = {
    [NONE] = "None",
    [QUEUED] = "Queued",
    [TRANSFERING_CACHE] = "Cache-to-Cache Transfer",
    [TRANSFERING_MEMORY] = "Memory Transfer",
    [WAITING_CACHE] = "Waiting for Cache",
    [WAITING_MEMORY] = "Waiting for Memory",
};

static const char* req_type_map[]
    = {[NO_REQ] = "None", [READSHARED] = "BusRd",   [READEX] = "BusRdX",
       [DATA] = "Data",   [SHARED] = "Shared", [MEMORY] = "Memory"};


const int CACHE_DELAY = 10;
const int CACHE_TRANSFER = 10;

// void registerCoher(coher* cc);
// void busReq(bus_req_type brt, uint64_t addr, int procNum);
// int busReqCacheTransfer(uint64_t addr, int procNum);
void printInterconnState(void);
void interconnNotifyState(void);

using namespace std;

snoop_recipients check_sharers(uint64_t addr){
    snoop_recipients sharers ;
    for(int pNum=0; pNum<processorCount;pNum++){
        void * treestate = tree_find(coherStates[pNum], addr);
        // *reinterpret_cast<double*>(&treestate)
        sharers.push_back(*reinterpret_cast<coherence_states*>(treestate));
    }
    assert(sharers.size() == processorCount); 
    return sharers; 
} 
// Helper methods for per-processor request queues.
static void enqBusRequest(bus_req* pr, int procNum)
{
    bus_req* iter;

    // No items in the queue.
    if (!queuedRequests[procNum])
    {
        queuedRequests[procNum] = pr;
        return;
    }

    // Add request to the end of the queue.
    iter = queuedRequests[procNum];
    while (iter->next)
    {
        iter = iter->next;
    }

    pr->next = NULL;
    iter->next = pr;
}

static bus_req* deqBusRequest(int procNum)
{
    bus_req* ret;

    ret = queuedRequests[procNum];

    // Move the head to the next request (if there is one).
    if (ret)
    {
        queuedRequests[procNum] = ret->next;
    }

    return ret;
}

static int busRequestQueueSize(int procNum)
{
    int count = 0;
    bus_req* iter;

    if (!queuedRequests[procNum])
    {
        return 0;
    }

    iter = queuedRequests[procNum];
    while (iter)
    {
        iter = iter->next;
        count++;
    }

    return count;
}


extern "C" void init_cpp(inter_sim_args* isa, interconn* self_c)
{
    int op;

    while ((op = getopt(isa->arg_count, isa->arg_list, "v")) != -1)
    {
        switch (op)
        {
            default:
                break;
        }
    }

    queuedRequests = malloc(sizeof(bus_req*) * processorCount);
    for (int i = 0; i < processorCount; i++)
    {
        queuedRequests[i] = NULL;
    }

    self = self_c;
    memComp = isa->memory;
    memComp->registerInterconnect(self);
    numSnoops.resize(processorCount, 0);

}

int countDown = 0;
int lastProc = 0; // for round robin arbitration

extern "C" void busReq_cpp(bus_req_type brt, uint64_t addr, int procNum){
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

        return;
    }
    else if (brt == SHARED && pendingRequest->addr == addr)
    {
        pendingRequest->shared = 1;
        return;
    }
    else if (brt == DATA && pendingRequest->addr == addr)
    {
        assert(pendingRequest->currentState == WAITING_MEMORY);
        pendingRequest->data = 1;
        pendingRequest->currentState = TRANSFERING_CACHE;
        countDown = CACHE_TRANSFER;
        return;
    }
    else
    {
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


extern "C" void registerCoher_cpp(coher* cc, void ** cohStateTree)
{
    
    coherComp = cc;
    coherStates = (tree_t**) cohStateTree;
}

void memReqCallback(int procNum, uint64_t addr)
{
    if (!pendingRequest)
    {
        return;
    }

    if (addr == pendingRequest->addr && procNum == pendingRequest->procNum)
    {
        pendingRequest->dataAvail = 1;
    }
}


extern "C" int tick_cpp()
{
    memComp->si.tick();

    if (self->dbgEnv.cadssDbgWatchedComp && !self->dbgEnv.cadssDbgNotifyState)
    {
        printInterconnState();
    }

    if (countDown > 0)
    {
        assert(pendingRequest != NULL);
        countDown--;

        // If the count-down has elapsed (or there hasn't been a
        // cache-to-cache transfer, the memory will respond with
        // the data.
        if (pendingRequest->dataAvail)
        {
            pendingRequest->currentState = TRANSFERING_MEMORY;
            countDown = 0;
        }

        if (countDown == 0)
        {
            if (pendingRequest->currentState == WAITING_CACHE)
            {
                // Make a request to memory.
                countDown
                    = memComp->busReq(pendingRequest->addr,
                                      pendingRequest->procNum, memReqCallback);

                pendingRequest->currentState = WAITING_MEMORY;

                // The processors will snoop for this request as well.
                // for (int i = 0; i < processorCount; i++)
                // {
                //     if (pendingRequest->procNum != i)
                //     {
                //         coherComp->busReq(pendingRequest->brt,
                //                           pendingRequest->addr, i);
                //     }
                // }
                snoop_recipients sharers = check_sharers(pendingRequest->addr);
                for(int i=0; i<sharers.size(); ++i){
                    if(sharers[i]!= INVALID && sharers[i] != UNDEF){
                        numSnoops[i]++;
                        coherComp->busReq(pendingRequest->brt,
                                          pendingRequest->addr, i);
                    }
                }

                if (pendingRequest->data == 1)
                {
                    pendingRequest->brt = DATA;
                }
            }
            else if (pendingRequest->currentState == TRANSFERING_MEMORY)
            {
                bus_req_type brt
                    = (pendingRequest->shared == 1) ? SHARED : DATA;
                coherComp->busReq(brt, pendingRequest->addr,
                                  pendingRequest->procNum);

                interconnNotifyState();
                free(pendingRequest);
                pendingRequest = NULL;
            }
            else if (pendingRequest->currentState == TRANSFERING_CACHE)
            {
                bus_req_type brt = pendingRequest->brt;
                if (pendingRequest->shared == 1)
                    brt = SHARED;

                coherComp->busReq(brt, pendingRequest->addr,
                                  pendingRequest->procNum);

                interconnNotifyState();
                free(pendingRequest);
                pendingRequest = NULL;
            }
        }
    }
    else if (countDown == 0)
    {
        for (int i = 0; i < processorCount; i++)
        {
            int pos = (i + lastProc) % processorCount;
            if (queuedRequests[pos] != NULL)
            {
                pendingRequest = deqBusRequest(pos);
                countDown = CACHE_DELAY;
                pendingRequest->currentState = WAITING_CACHE;

                lastProc = (pos + 1) % processorCount;
                break;
            }
        }
    }

    return 0;
}

void printInterconnState(void)
{
    if (!pendingRequest)
    {
        return;
    }

    printf("--- Interconnect Debug State (Processors: %d) ---\n"
           "       Current Request: \n"
           "             Processor: %d\n"
           "               Address: 0x%016lx\n"
           "                  Type: %s\n"
           "                 State: %s\n"
           "         Shared / Data: %s\n"
           "                  Next: %p\n"
           "             Countdown: %d\n"
           "    Request Queue Size: \n",
           processorCount, pendingRequest->procNum, pendingRequest->addr,
           req_type_map[pendingRequest->brt],
           req_state_map[pendingRequest->currentState],
           pendingRequest->shared ? "Shared" : "Data", pendingRequest->next,
           countDown);

    for (int p = 0; p < processorCount; p++)
    {
        printf("       - Processor[%02d]: %d\n", p, busRequestQueueSize(p));
    }
}

void interconnNotifyState(void)
{
    if (!pendingRequest)
        return;

    if (self->dbgEnv.cadssDbgExternBreak)
    {
        printInterconnState();
        raise(SIGTRAP);
        return;
    }

    if (self->dbgEnv.cadssDbgWatchedComp && self->dbgEnv.cadssDbgNotifyState)
    {
        self->dbgEnv.cadssDbgNotifyState = 0;
        printInterconnState();
    }
}

// Return a non-zero value if the current request
// was satisfied by a cache-to-cache transfer.
extern "C" int busReqCacheTransfer_cpp(uint64_t addr, int procNum)
{
    assert(pendingRequest);

    if (addr == pendingRequest->addr && procNum == pendingRequest->procNum)
        return (pendingRequest->currentState == TRANSFERING_CACHE);

    return 0;
}

extern "C" int finish_cpp(int outFd){
    memComp->si.finish(0);
    FILE* file = fdopen(outFd, "w");
    if(file != nullptr){
        std::ostringstream oss;
        oss << "Inteconnect Stats "<<std::endl;
        oss << "Number of Snoops Recieved"<<std::endl;
        for(int i=0; i<processorCount; ++i){
            oss<<"  Core "<<i<<": "<<numSnoops[i]<<std::endl;
        }
        fwrite(oss.str().c_str(), sizeof(char), oss.str().size(), file);
        fclose(file);
    }else{
        std::cout<<"Unable to open interconnect file to write stats"<<endl;
    }

    return 0;
}

extern "C" int destroy_cpp(void)
{
    // TODO
    memComp->si.destroy();
    return 0;
}