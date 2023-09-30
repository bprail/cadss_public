#include <stdio.h>
#include <getopt.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <trace.h>
#include <processor.h>
#include <branch.h>
#include <unistd.h>
#include <libgen.h>

#include "config.h"
#include "engine.h"

int CADSS_VERBOSE = 0;
int processorCount = 1;

void printHelp(char* prog)
{
    printf("%s \n", prog);
    printf("  -h          \t Help message\n");
    printf("  -v          \t Verbose\n");
    printf("  -n <num>    \t Number of processors to simulate\n");
    printf("  -c <file>   \t Cache simulator\n");
    printf("  -p <file>   \t Pipeline simulator\n");
    printf("  -o <file>   \t Coherence simulator\n");
    printf("  -i <file>   \t Interconnection simulator\n");
    printf("  -b <file>   \t Branch simulator\n");
    printf("  -m <file>   \t Memory simulator\n");
    printf("  -t <file>   \t Trace file / directory\n");
    printf("  -s <file>   \t Setting / configuration file\n");
    printf("  -d [<tick>] \t Enable debugging\n"
           "              \t  - drops into a debug REPL\n"
           "              \t  - if <tick> specified, waits for <tick>\n"
           "              \t    cycles to elapse before prompy"
           "              \t  - if run with external debugger, state\n"
           "              \t    changes deliver SIGTRAP\n");
}

//
// loadSim (name, type)
//    Attempts to load "name/libname.so"
//
struct sim* loadSim(char* name, char* type)
{
    char* baseName = NULL;
    char fullName[SIM_NAME_LIMIT] = {0};
    ssize_t len;

    // TODO
    //  - Support for alternate naming schemes
    //  - if debug == 1, try loading a -debug.so
    baseName = basename(name);

    len = snprintf(fullName, SIM_NAME_LIMIT, "%s/lib%s.so", name, baseName);
    if (len == SIM_NAME_LIMIT || len < 0)
    {
        fprintf(stderr,
                "Failed to generate so name for %s component using %s\n", type,
                name);

        return NULL;
    }

    void* handle = dlopen(fullName, RTLD_LAZY);
    if (handle == NULL)
    {
        fprintf(stderr, "Failed to load %s component using %s: %s\n", type,
                fullName, dlerror());
        return NULL;
    }

    struct sim* s = malloc(sizeof(struct sim));
    if (s == NULL)
    {
        dlclose(handle);
        fprintf(stderr, "Failed to allocate space for %s component\n", type);
        return NULL;
    }

    s->handle = handle;
    s->init = dlsym(handle, "init");
    s->tick = dlsym(handle, "tick");
    s->finish = dlsym(handle, "finish");
    s->destroy = dlsym(handle, "destroy");
    s->CADSS_VERBOSE = dlsym(handle, "CADSS_VERBOSE");
    if (s->CADSS_VERBOSE != NULL)
    {
        *(s->CADSS_VERBOSE) = CADSS_VERBOSE;
    }

    int* pCount = dlsym(handle, "processorCount");
    if (pCount != NULL)
    {
        *pCount = processorCount;
    }

    if (s->init == NULL || s->tick == NULL || s->finish == NULL
        || s->destroy == NULL)
    {
        dlclose(handle);
        free(s);
        fprintf(stderr, "Failed to load interface for %s component\n", type);
        return NULL;
    }
    return s;
}

// Handle debug REPL prompts.
static int debugRepl(int64_t tickCount)
{
    char line[MAX_DBG_LINE_BUF_SIZE + 1] = {0};
    enum dbgCmd cmd;
    int showPrompt = 1;

    // Disable cadss debug REPL if the engine
    // is invoked with an external debugger.
    if (CADSS_DBG_EXT)
        return 0;

    // If "-d <tick>", then turn on debugging after that tick.
    if (CADSS_DBG_TICK >= 0 && tickCount >= CADSS_DBG_TICK)
    {
        CADSS_DBG_ON = 1;
    }

    if (!CADSS_DBG_ON)
        return 0;

    if (CADSS_DBG_NOTIF)
        return 0;

    // Step through.
    if (--CADSS_DBG_STEP_TICKS > 0)
    {
        return 0;
    }

    // REPL.
    while (showPrompt)
    {
        memset(line, 0x0, MAX_DBG_LINE_BUF_SIZE + 1);

        // Display prompt, get input.
        if (tickCount)
            printf("Tick: %ld\n", tickCount);

        printf("> ");
        if (fgets(line, MAX_DBG_LINE_BUF_SIZE, stdin) != line)
            continue;

        line[strcspn(line, "\n")] = '\0';
        cmd = parseDebugReplCmd(line);

        if (cmd == CMD_HLT)
            return 1;

        showPrompt = handleDbgReplCmd(cmd, line);
    }

    return 0;
}

static void debugInitEnv(debug_env_vars* compEnv)
{
    compEnv->cadssDbgNotifyState = 0;
    compEnv->cadssDbgWatchedComp = 0;
    compEnv->cadssDbgExternBreak = CADSS_DBG_EXT;
}

static void debugWatchComponent(debug_env_vars* compEnv, uint8_t mask)
{
    compEnv->cadssDbgWatchedComp = !!(CADSS_DBG_WLIST_STATE & mask);

    if (compEnv->cadssDbgWatchedComp)
        compEnv->cadssDbgNotifyState = CADSS_DBG_NOTIF;
}

static void debugCheckNotif(debug_env_vars* compEnv)
{
    if (compEnv->cadssDbgWatchedComp)
        CADSS_DBG_NOTIF &= compEnv->cadssDbgNotifyState;
}

int main(int argc, char** argv)
{
    int opt;
    struct sim* csim = NULL;
    struct sim* psim = NULL;
    struct sim* bsim = NULL;
    struct sim* isim = NULL;
    struct sim* osim = NULL;
    struct sim* trace = NULL;
    struct sim* msim = NULL;
    char* settingFile = NULL;
    char* cacheName = NULL;
    char* branchName = NULL;
    char* procName = NULL;
    char* coherName = NULL;
    char* interName = NULL;
    char* memName = NULL;

    // TODO - switch to getopt_long that accepts -- arguments
    while ((opt = getopt(argc, argv, ":hvc:p:o:n:i:b:t:s:m:d:")) != -1)
    {
        switch (opt)
        {
            case 'd':
                CADSS_DBG_TICK = atoi(optarg);
                if (CADSS_DBG_TICK >= 0)
                    CADSS_DBG_ON = 0;
                else
                    CADSS_DBG_TICK = -1;

                break;
            case 'h':
                printHelp(argv[0]);
                return 0;
            case 'p':
                procName = optarg;
                break;
            case 'v':
                CADSS_VERBOSE = 1;
                break;
            case 'c':
                cacheName = optarg;
                break;
            case 'b':
                branchName = optarg;
                break;
            case 's':
                settingFile = optarg;
                break;
            case 'o':
                coherName = optarg;
                break;
            case 'n':
                processorCount = atoi(optarg);
                break;
            case 'i':
                interName = optarg;
                break;
            case 'm':
                memName = optarg;
                break;
            case ':':
                if (optopt == 'd')
                {
                    CADSS_DBG_ON = 1;
                }
                break;
        }
    }

    if (isProcTracedExt() && CADSS_DBG_ON)
        CADSS_DBG_EXT = 1;

    trace = loadSim("trace", "trace");
    trace_sim_args tsa;
    tsa.arg_count = argc;
    tsa.arg_list = argv;
    optind = 1;
    trace_reader* tr = trace->init(&tsa);

    if (settingFile == NULL)
    {
        fprintf(stderr, "No setting file specified, using default.config\n");
        settingFile = "default.config";
    }
    if (openSettings(settingFile) != 0)
    {
        fprintf(stderr, "Failed to open setting file - %s\n", settingFile);
        return 0;
    }

    if (interName == NULL)
    {
        isim = loadSim("interconnect", "interconnect");
    }
    else
    {
        isim = loadSim(interName, "interconnect");
        if (isim == NULL)
        {
            return 0;
        }
    }

    if (coherName == NULL)
    {
        osim = loadSim("coherence", "coherence");
    }
    else
    {
        osim = loadSim(coherName, "coherence");
        if (osim == NULL)
        {
            return 0;
        }
    }

    if (cacheName == NULL)
    {
        csim = loadSim("cache", "cache");
    }
    else
    {
        csim = loadSim(cacheName, "cache");
        if (csim == NULL)
        {
            return 0;
        }
    }

    if (procName == NULL)
    {
        psim = loadSim("processor", "processor");
    }
    else
    {
        psim = loadSim(procName, "processor");
        if (psim == NULL)
        {
            return 0;
        }
    }

    if (branchName == NULL)
    {
        bsim = loadSim("branch", "branch");
    }
    else
    {
        bsim = loadSim(branchName, "branch");
        if (bsim == NULL)
        {
            return 0;
        }
    }

    if (memName == NULL)
    {
        msim = loadSim("memory", "memory");
    }
    else
    {
        msim = loadSim(memName, "memory");
        if (msim == NULL)
        {
            return 0;
        }
    }

    // B.N. - set optind to 1 before calling init on any component
    //  that resets getopt() so the component can use it safely on its arguments
    int argCount = 0;
    char** arg = NULL;

    cache* cache_sim = NULL;
    branch* branch_sim = NULL;
    coher* coher_sim = NULL;
    interconn* inter_sim = NULL;
    memory* mem_sim = NULL;
    processor* proc_sim = NULL;

    arg = getSettings("memory", &argCount);
    if (arg == NULL) {}

    memory_sim_args msa;
    msa.arg_count = argCount;
    msa.arg_list = arg;
    if ((mem_sim = msim->init(&msa)) != 0) {}

    arg = getSettings("interconnect", &argCount);
    if (arg == NULL) {}

    inter_sim_args isa;
    isa.arg_count = argCount;
    isa.arg_list = arg;
    isa.memory = mem_sim;
    optind = 1;
    if ((inter_sim = isim->init(&isa)) == NULL) {}

    arg = getSettings("coherence", &argCount);
    if (arg == NULL) {}

    coher_sim_args osa;
    osa.arg_count = argCount;
    osa.arg_list = arg;
    osa.inter = inter_sim;
    optind = 1;
    if ((coher_sim = osim->init(&osa)) == NULL) {}

    optind = 1;
    arg = getSettings("cache", &argCount);
    if (arg == NULL) {}

    cache_sim_args csa;
    csa.arg_count = argCount;
    csa.arg_list = arg;
    csa.coherComp = coher_sim;
    if ((cache_sim = csim->init(&csa)) == NULL) {}

    optind = 1;
    arg = getSettings("branch", &argCount);
    if (arg == NULL) {}

    branch_sim_args bsa;
    bsa.arg_count = argCount;
    bsa.arg_list = arg;
    if ((branch_sim = bsim->init(&bsa)) == NULL) {}

    optind = 1;
    arg = getSettings("processor", &argCount);
    if (arg == NULL) {}

    processor_sim_args psa;
    psa.arg_count = argCount;
    psa.arg_list = arg;
    psa.tr = tr;
    psa.cache_sim = cache_sim;
    psa.branch_sim = branch_sim;
    if ((proc_sim = psim->init(&psa)) == NULL)
    {
        printf("Failed to initialize processor!\n");
        assert(0);
    }

    // Main sim loop
    int progress = 0;
    int dbgHalt;
    int64_t dbgTickCount = 0;

    debugInitEnv(&(proc_sim->dbgEnv));
    debugInitEnv(&(branch_sim->dbgEnv));
    debugInitEnv(&(cache_sim->dbgEnv));
    debugInitEnv(&(coher_sim->dbgEnv));
    debugInitEnv(&(inter_sim->dbgEnv));
    debugInitEnv(&(mem_sim->dbgEnv));

    do
    {
        dbgHalt = debugRepl(dbgTickCount);
        if (dbgHalt)
            break;

        // Mark components to watch and notify state changes.
        debugWatchComponent(&(proc_sim->dbgEnv), CADSS_DBG_WATCH_PROC);
        debugWatchComponent(&(branch_sim->dbgEnv), CADSS_DBG_WATCH_BRANCH);
        debugWatchComponent(&(cache_sim->dbgEnv), CADSS_DBG_WATCH_CACHE);
        debugWatchComponent(&(coher_sim->dbgEnv), CADSS_DBG_WATCH_COHER);
        debugWatchComponent(&(inter_sim->dbgEnv), CADSS_DBG_WATCH_INTER);
        debugWatchComponent(&(mem_sim->dbgEnv), CADSS_DBG_WATCH_MEM);

        // Processor requests trace ops as needed.
        progress = psim->tick();
        dbgTickCount++;

        // Check if any of the watched components
        // requested to be notified of state change.
        debugCheckNotif(&(proc_sim->dbgEnv));
        debugCheckNotif(&(branch_sim->dbgEnv));
        debugCheckNotif(&(cache_sim->dbgEnv));
        debugCheckNotif(&(coher_sim->dbgEnv));
        debugCheckNotif(&(inter_sim->dbgEnv));
        debugCheckNotif(&(mem_sim->dbgEnv));
    } while (progress);

    psim->finish(STDOUT_FILENO);
    psim->destroy();
    trace->destroy();

    dlclose(csim->handle);
    free(csim);
    dlclose(psim->handle);
    free(psim);
    dlclose(bsim->handle);
    free(bsim);
    dlclose(trace->handle);
    free(trace);
    dlclose(msim->handle);
    free(msim);

    return 0;
}
