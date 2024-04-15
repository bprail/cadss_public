#include <branch.h>
#include <trace.h>

#include <getopt.h>
#include <stdlib.h>
#include <assert.h>

#include "br.h"

branch* self = NULL;

uint64_t branchRequest(trace_op* op, int processorNum);

branch* init(branch_sim_args* csa)
{
    int op;
    
    // TODO - get argument list from assignment
    while ((op = getopt(csa->arg_count, csa->arg_list, "p:")) != -1)
    {
        switch (op)
        {
            // Processor count
            case 'p':
                
                break;
        }
    }
    
    self = malloc(sizeof(branch));
    self->branchRequest = branchRequestCPP;
    self->si.tick = tick;
    self->si.finish = finish;
    self->si.destroy = destroy;
    
    initCPP();
    
    return self;
}

int tick()
{
    
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
