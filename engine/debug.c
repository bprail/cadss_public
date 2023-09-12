#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "engine.h"

uint8_t CADSS_DBG_WLIST_STATE = 0;
int CADSS_DBG_ON = 0;
int CADSS_DBG_STEP_TICKS = 0;

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
