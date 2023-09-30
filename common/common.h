#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Every component needs to define the following:
int tick();
int finish(int);
int destroy(void);

// Every componet also needs to define an init that returns
//   a pointer specific to that type of component

typedef struct _sim_interface {
    int (*tick)(void);
    int (*finish)(int);
    int (*destroy)(void);
} sim_interface;

// Flag set by engine if verbose flag is passed in,
// components can read flag
extern int CADSS_VERBOSE;
extern int processorCount;

// For cadss debugging functionality.
typedef struct _debug_env_vars {
    int cadssDbgWatchedComp;
    int cadssDbgNotifyState;
    int cadssDbgExternBreak;
} debug_env_vars;

#endif
