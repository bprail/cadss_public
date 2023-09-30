#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>

#include "engine.h"

int CADSS_DBG_ON = 0;
int64_t CADSS_DBG_TICK = -1;
int CADSS_DBG_NOTIF = 0;
uint8_t CADSS_DBG_WLIST_STATE = 0;
int CADSS_DBG_STEP_TICKS = 0;
int CADSS_DBG_EXT = 0;

static void updateDebugWlist(const char* args, uint8_t watch)
{
    uint8_t flag = 0;

    for (int i = 0; args[i] != '\0'; i++)
    {
        flag = 0;

        switch (args[i])
        {
            case 'p':
                flag = CADSS_DBG_WATCH_PROC;
                break;
            case 'b':
                flag = CADSS_DBG_WATCH_BRANCH;
                break;
            case 'c':
                flag = CADSS_DBG_WATCH_CACHE;
                break;
            case 'o':
                flag = CADSS_DBG_WATCH_COHER;
                break;
            case 'i':
                flag = CADSS_DBG_WATCH_INTER;
                break;
            case 'm':
                flag = CADSS_DBG_WATCH_MEM;
                break;
        }

        if (watch)
            CADSS_DBG_WLIST_STATE |= flag;
        else
            CADSS_DBG_WLIST_STATE &= ~flag;
    }
}

static void printDebugHelp()
{
    printf("-- cadss-engine: Debug REPL --\n"
           "    Available Commands:\n"
           "     w [arg]  Add component(s) to watch-list.\n"
           "              arg: \"[pbcoim]+\"\n\n"
           "     i [arg]  Remove component(s) from watch-list.\n"
           "              arg: \"[pbcoim]+\"\n\n"
           "     n [arg]  Advance simulation by by \"arg\" ticks.\n"
           "              arg: T, where T > 0\n"
           "              If unspecified, advances by 1 tick.\n\n"
           "     c        Continue execution until state change.\n\n"
           "     e        Exit from debug REPL.\n\n"
           "     q        Quit (exit and halt) the simulation.\n\n"
           "     l        List watched components.\n\n"
           "     h        Show help message.\n");
}

static void printDebugWatchedComps()
{
    printf("Watched Components: [pbcoim]\n"
           "                    [%d%d%d%d%d%d]\n",
           !!(CADSS_DBG_WLIST_STATE & CADSS_DBG_WATCH_PROC),
           !!(CADSS_DBG_WLIST_STATE & CADSS_DBG_WATCH_BRANCH),
           !!(CADSS_DBG_WLIST_STATE & CADSS_DBG_WATCH_CACHE),
           !!(CADSS_DBG_WLIST_STATE & CADSS_DBG_WATCH_COHER),
           !!(CADSS_DBG_WLIST_STATE & CADSS_DBG_WATCH_INTER),
           !!(CADSS_DBG_WLIST_STATE & CADSS_DBG_WATCH_MEM));
}
enum dbgCmd parseDebugReplCmd(const char* cmdStr)
{
    switch (cmdStr[0])
    {
        case 'w':
            return CMD_WAT;
        case 'i':
            return CMD_IGN;
        case 'n':
            return CMD_NXT;
        case 'c':
            return CMD_CON;
        case 'e':
            return CMD_EXT;
        case 'q':
            return CMD_HLT;
        case 'h':
            return CMD_HLP;
        case 'l':
            return CMD_LST;
        default:
            return CMD_ERR;
    }
}

int handleDbgReplCmd(enum dbgCmd cmd, const char* cmdStr)
{
    // Prompt (back) on, swich off continue.
    CADSS_DBG_NOTIF = 0;

    if (cmdStr[0] == '\0')
        return 1;

    switch (cmd)
    {
        case CMD_NXT:
            CADSS_DBG_STEP_TICKS = atoi(cmdStr + 1);
            if (CADSS_DBG_STEP_TICKS <= 0)
                CADSS_DBG_STEP_TICKS = 1;

            return 0;

        case CMD_EXT:
            CADSS_DBG_ON = 0;
            CADSS_DBG_WLIST_STATE = 0;

            return 0;

        case CMD_CON:
            CADSS_DBG_NOTIF = 1;
            return 0;

        case CMD_WAT:
            updateDebugWlist(cmdStr + 1, 1);
            break;

        case CMD_IGN:
            updateDebugWlist(cmdStr + 1, 0);
            break;

        case CMD_HLP:
            printDebugHelp();
            break;

        case CMD_LST:
            printDebugWatchedComps();
            break;

        case CMD_HLT:
            CADSS_DBG_ON = 0;
            CADSS_DBG_WLIST_STATE = 0;
            break;

        default:
            printf("Invalid command; use 'h' to display usage.\n");
            break;
    }

    return 1;
}

// Check if the simulator was started with an
// external debugger (like "gdb", "lldb", etc.).
int isProcTracedExt()
{
    pid_t chldPID, prntPID;
    int status;

    chldPID = fork();
    assert(chldPID >= 0);

    if (chldPID == 0)
    {
        prntPID = getppid();

        // From the child, try to attach to the parent and
        // trace it. If it fails, the parent is being traced.
        // If it doesn't then wait for the parent to stop (it
        // stops because the child attached to it), make it
        // set to to continye and detach.
        if (ptrace(PTRACE_ATTACH, prntPID, NULL, NULL) == -1)
            exit(1);

        waitpid(prntPID, NULL, 0);
        ptrace(PTRACE_CONT, NULL, NULL);
        ptrace(PTRACE_DETACH, getppid(), NULL, NULL);

        exit(0);
    }

    // Parent (patiently) wait for the child.
    waitpid(chldPID, &status, 0);

    return WEXITSTATUS(status);
}
