#ifdef __cplusplus
#include <cstdint>
#include <cstdio>

extern "C" {
#endif

#include <stdio.h>
#include <trace.h>

int8_t initTaskGraph(FILE*);
trace_op* getNextOp(int processorNum);

#ifdef __cplusplus
}
#endif