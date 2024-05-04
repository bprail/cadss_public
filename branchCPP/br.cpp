#include <vector>

#include <iostream>

#include "br.h"

void initCPP()
{
    std::cout << "INIT\n" << std::endl;
}

uint64_t branchRequestCPP(trace_op* op, int processorNum)
{
    std::cout << "TEST\n" << std::endl;
    return 1;
}