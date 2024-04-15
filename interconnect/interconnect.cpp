#include <getopt.h>
#include <stdio.h>
#include <memory.h>
#include <iostream>
#include <interconnect_internal.h>

using namespace std;
extern "C" int finish_cpp(int outFd, memory* memComp){
    memComp->si.finish(outFd);
    std::cout<<"Hi from CPP using cout"<<endl;
    return 0;
}