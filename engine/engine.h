#ifndef ENGINE_H
#define ENGINE_H

#define SIM_NAME_LIMIT 256

struct sim {
    void* handle;
    void* (*init)(void*);
    int (*tick)(void);
    int (*finish)(int);
    int (*destroy)(void);
    int *CADSS_VERBOSE;
} ;

#endif
