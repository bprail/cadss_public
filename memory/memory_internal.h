#ifndef MEMORY_INTERNAL_H
#define MEMORY_INTERNAL_H

// Describes a DRAM request.
typedef struct _memReq {
    int procNum;
    uint64_t addr;
    int squelch;
    void (*callback)(int, uint64_t);
} memReq;

#endif // MEMORY_INTERNAL_H
