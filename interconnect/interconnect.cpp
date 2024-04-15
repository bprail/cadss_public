#include <getopt.h>
#include <stdio.h>
#include <memory.h>
#include <interconnect_internal.h>


extern "C" int finish_cpp(int outFd, memory* memComp){
    memComp->si.finish(outFd);
    return 0;
}