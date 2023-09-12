#ifndef ENGINE_H
#define ENGINE_H

#define SIM_NAME_LIMIT 256

// Line buffer size for REPL command line.
#define MAX_DBG_LINE_BUF_SIZE  31

// For variable length "scanf" specifier.
#define _SCAN_STR(x) #x
#define SCANF_STR(x) _SCAN_STR(x)

// A bit for each of the components to be watched.
#define CADSS_DBG_WATCH_PROC   (0x1U << 5)
#define CADSS_DBG_WATCH_BRANCH (0x1U << 4)
#define CADSS_DBG_WATCH_CACHE  (0x1U << 3)
#define CADSS_DBG_WATCH_COHER  (0x1U << 2)
#define CADSS_DBG_WATCH_INTER  (0x1U << 1)
#define CADSS_DBG_WATCH_MEM    (0x1U << 0)

// Describes the debug command specified.
enum dbgCmd {
    CMD_ERR, // --  Error
    CMD_WAT, // w:  Watch
    CMD_IGN, // i:  Ignore
    CMD_NXT, // n:  Next
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
    int *CADSS_VERBOSE;
};

extern int CADSS_DBG_ON;
extern uint8_t CADSS_DBG_WLIST_STATE;
extern int CADSS_DBG_STEP_TICKS;

enum dbgCmd parseDebugReplCmd(const char *cmdStr);
int handleDbgReplCmd(enum dbgCmd cmd, const char* cmdStr);

#endif
