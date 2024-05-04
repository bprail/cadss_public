#ifdef __cplusplus
#include <cstdint>

extern "C" {
#endif

#include <branch.h>
#include <trace.h>
 
uint64_t branchRequestCPP(trace_op* op, int processorNum);

void initCPP();

#ifdef __cplusplus
}
#endif