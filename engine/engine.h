#ifndef ENGINE_H
#define ENGINE_H

#define SIM_NAME_LIMIT 256

// Line buffer size for REPL command line.
#define MAX_DBG_LINE_BUF_SIZE 31

// A bit for each of the components to be watched.
#define CADSS_DBG_WATCH_PROC (0x1U << 5)
#define CADSS_DBG_WATCH_BRANCH (0x1U << 4)
#define CADSS_DBG_WATCH_CACHE (0x1U << 3)
#define CADSS_DBG_WATCH_COHER (0x1U << 2)
#define CADSS_DBG_WATCH_INTER (0x1U << 1)
#define CADSS_DBG_WATCH_MEM (0x1U << 0)

// Describes the debug command specified.
enum dbgCmd
{
    CMD_ERR, // --  Error
    CMD_WAT, // w:  Watch
    CMD_IGN, // i:  Ignore
    CMD_NXT, // n:  Next
    CMD_CON, // c:  Continue
    CMD_EXT, // e:  Exit
    CMD_HLT, // q:  Quit
    CMD_HLP, // h:  Help
    CMD_LST, // l:  List
};

struct sim {
    void* handle;
    void* (*init)(void*);
    int (*tick)(void);
    int (*finish)(int);
    int (*destroy)(void);
    int* CADSS_VERBOSE;
};

// Engine started with "-d".
extern int CADSS_DBG_ON;

// Engine startied with "-d <ticks>".
extern int64_t CADSS_DBG_TICK;

// For the "c" command where components
// notify the engine of state change.
extern int CADSS_DBG_NOTIF;

// List of watched components.
extern uint8_t CADSS_DBG_WLIST_STATE;

// Number of ticks to step though.
extern int CADSS_DBG_STEP_TICKS;

// If the program is externally traced.
extern int CADSS_DBG_EXT;

enum dbgCmd parseDebugReplCmd(const char* cmdStr);
int handleDbgReplCmd(enum dbgCmd cmd, const char* cmdStr);
int isProcTracedExt(void);

#endif
