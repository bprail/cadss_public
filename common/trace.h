#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>

#include "common.h"

//
// TRACE H - Public interface to trace file reader
//

enum op_type
{
    NONE,
    MEM_LOAD,
    MEM_STORE,
    BRANCH,
    ALU,
    ALU_LONG,
    END
};

typedef int proc_id;

typedef struct _trace_op {
    enum op_type op;
    int dest_reg;
    int src_reg[2];
    uint64_t pcAddress;
    union {
        uint64_t memAddress;
        uint64_t nextPCAddress;
    };
    int size;
} trace_op;

typedef struct _trace_sim_args {
    int arg_count;
    char** arg_list;
} trace_sim_args;

typedef struct _trace_reader {
    sim_interface si;
    trace_op* (*getNextOp)(int);
} trace_reader;

#endif
