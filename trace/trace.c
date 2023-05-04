#include "trace.h"
#include "trace_internal.h"
#include "taskLib/TaskGraphAPI.h"

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dlfcn.h>

trace_op* getNextOp(int);

int processorCount = 1;

FILE** traceFile = NULL;
int masterFD = 0;

int8_t isTaskGraph = 0;
trace_op* (*gno)(int processorNum) = NULL;

trace_reader* init(trace_sim_args* tsa)
{
    char* trace = NULL;
    trace_reader* tr = malloc(sizeof(trace_reader));
    if (tr == NULL) return NULL;
    tr->getNextOp = getNextOp;
    
    int op = 0;
    while ((op = getopt(tsa->arg_count, tsa->arg_list, "hdvc:p:o:n:i:b:t:s:m:")) != -1)
    {
        switch (op)
        {
            case 't':
                trace = optarg;
                break;
        }
    }
    
    traceFile = calloc(processorCount, sizeof(FILE*));
    
    if (trace == NULL)
    {
        fprintf(stderr, "No trace file / directory specified, continuing using stdin\n");
        traceFile[0] = stdin;
    }
    else
    {
        masterFD = open(trace, O_DIRECTORY);
        if (masterFD == -1)
        {
            traceFile[0] = fopen(trace, "rb");
            if (traceFile[0] == NULL)
            {
                perror("Attempt to open trace file");
                fprintf(stderr, "Failed on trace file name - %s\n", optarg);
                free(tr);
                return NULL;
            }
            
            // Is this file a taskgraph?
            //   Taskgraphs are compressed binary traces
            //   and require a separate C++ library to process
            //   c.f. github.com/bprail/contech
            int traceNameLen = strlen(trace);
            if (strcmp("taskgraph", &trace[traceNameLen - 9]) == 0)
            {
                void* handle = dlopen("trace/taskLib/libtaskLib.so", RTLD_LAZY);
                int8_t (*itg)(FILE*) = dlsym(handle, "initTaskGraph");
                if (itg != NULL)
                {
                    isTaskGraph = itg(traceFile[0]);
                }
                else
                {
                    
                }
                
                gno = dlsym(handle, "getNextOp");
            }
        }
        
        // openat()
    }
    
    tr->si.tick = tick;
    tr->si.finish = finish;
    tr->si.destroy = destroy;
    
    return tr;
}

uint64_t opCount = 0;
trace_op* getNextOp(int processorNum)
{
    trace_op* op = calloc(1, sizeof(trace_op));
    FILE* tf = NULL;
    
    if (isTaskGraph == 1)
    {
        return gno(processorNum);
    }
    
    if (traceFile[processorNum] == NULL)
    {
        char fileName[16];
        snprintf(fileName, 16, "p%d.trace", processorNum);
        
        int tempFD = openat(masterFD, fileName, O_RDONLY);
        if (tempFD == -1)
        {
            perror("Error opening processor specific trace - ");
            
            return NULL;
        }
        
        traceFile[processorNum] = fdopen(tempFD, "r");
        if (traceFile[processorNum] == NULL)
        {
            perror("Error converting FD for processor specific trace - ");
            
            return NULL;
        }
    }
    
    tf = traceFile[processorNum];
    
    // TODO - Support for other basic formats
    char opType = 0;
    uint64_t memAddress, pcAddress, nextPC;
    int opSize;
    int32_t op0, op1, op2;
    if (0 == fscanf(tf, "%c", &opType))
    {
        free(op);
        return NULL;
    }
    
    if (opType == '\0' || isspace(opType))
    {
        free(op);
        return NULL;
    }
    
    switch (opType)
    {
        case 'A':
            op->op = ALU;
            (void)!fscanf(tf, "%lx %d, %d, %d\n", &pcAddress, &op0, &op1, &op2);
            op->pcAddress = pcAddress;
            op->dest_reg = op0;
            op->src_reg[0] = op1;
            op->src_reg[1] = op2;
            break;
        case 'B':
            op->op = BRANCH;
            (void)!fscanf(tf, "%lx %lx", &pcAddress, &nextPC);
            if (1 == fscanf(tf, " %d\n", &op0))
            {
                op->src_reg[0] = op0;
            }
            else
            {
                (void)!fscanf(tf, "\n");
                op->src_reg[0] = -1;
            }
            op->pcAddress = pcAddress;
            op->nextPCAddress = nextPC;
            op->src_reg[1] = -1;
            op->dest_reg = -1;
            break;
        case 'L':
            op->op = MEM_LOAD;
            (void)!fscanf(tf, "%lx,%d", &memAddress, &opSize);
            if (1 == fscanf(tf, " %d\n", &op0))
            {
                op->src_reg[0] = op0;
            }
            else
            {
                (void)!fscanf(tf, "\n");
                op->src_reg[0] = -1;
            }
            op->memAddress = memAddress;
            op->size = opSize;
            op->src_reg[1] = -1;
            op->dest_reg = -1;
            break;
        case 'S':
            op->op = MEM_STORE;
            (void)!fscanf(tf, "%lx,%d", &memAddress, &opSize);
            if (1 == fscanf(tf, " %d\n", &op0))
            {
                op->dest_reg = op0;
            }
            else
            {
                (void)!fscanf(tf, "\n");
                op->dest_reg = -1;
            }
            op->memAddress = memAddress;
            op->size = opSize;
            op->src_reg[0] = -1;
            op->src_reg[1] = -1;
            break;
        case 'X':
            op->op = ALU_LONG;
            (void)!fscanf(tf, "%lx %d, %d, %d\n", &pcAddress, &op0, &op1, &op2);
            op->pcAddress = pcAddress;
            op->dest_reg = op0;
            op->src_reg[0] = op1;
            op->src_reg[1] = op2;
            break;
        default:
            fprintf(stderr, "Invalid op type: %x on %ld\n", opType, opCount);
            free(op);
            return NULL;
    }
    
    opCount++;
    return op;
}

int tick(void)
{
    return 1;    
}

int finish(int outFd)
{
    return 0;    
}

int destroy(void)
{
    int i;
    for (i = 0; i < processorCount; i++)
    {
        if (traceFile[i] != NULL) fclose(traceFile[i]);
    }
    return 0;
}
