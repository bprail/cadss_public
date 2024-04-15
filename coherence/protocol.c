#include "coher_internal.h"

// void sendBusRd(uint64_t addr, int procNum)
// {
//     inter_sim->busReq(BUSRD, addr, procNum);
// }

// void sendBusWr(uint64_t addr, int procNum)
// {
//     inter_sim->busReq(BUSWR, addr, procNum);
// }

// void sendData(uint64_t addr, int procNum)
// {
//     inter_sim->busReq(DATA, addr, procNum);
// }

// void indicateShared(uint64_t addr, int procNum)
// {
//     inter_sim->busReq(SHARED, addr, procNum);
// }

// Directory -> cache
void sendSnoopDowngrade(uint64_t addr, int procNum) {
    inter_sim->busReq(SNOOPDNGRADE, addr, procNum);
}

void sendSnoopInvalidate(uint64_t addr, int procNum) {
    inter_sim->busReq(SNOOPINV, addr, procNum);
}

void sendGrantExcl(uint64_t addr, int procNum) {
    inter_sim->busReq(GRANTEXCL, addr, procNum);
}

// Cache -> directory
void sendRead(uint64_t addr, int procNum) {
    inter_sim->busReq(READSHARED, addr, procNum);
}

void sendReadExcl(uint64_t addr, int procNum) {
    inter_sim->busReq(READEX, addr, procNum);
}

// Called when cache is hitting us, asking if it has permission
// In lecture diagrams, corresponds to Pr... messages
coherence_states
cacheMSI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum) {
    switch (currentState)
    {
        // We have access and nothing changes
        case MODIFIED:
            *permAvail = 1;
            return MODIFIED;
        // Access persists if read, otherwise upgrade
        case SHARED:
            if (is_read) {
                *permAvail = 1;
                return SHARED;
            } else {
                // I think there's no need to stall, since we have data
                *permAvail = 0;
                sendReadExcl(addr, procNum);
                return SHARED_MODIFIED;
            }
        // Get the value we need
        case INVALID:
            *permAvail = 0;
            if (is_read) {
                sendRead(addr, procNum);
                return SHARED;
            } else {
                sendReadExcl(addr, procNum);
                return INVALID_MODIFIED;
            }
        // Stall cases
        case SHARED_MODIFIED:
            fprintf(stderr, "SM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            return SHARED_MODIFIED;

        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            return INVALID_MODIFIED;

        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }
}

coherence_states
snoopMSI(dir_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum) {
    *ca = NO_ACTION;
    switch (currentState)
    {
        // Main states
        case MODIFIED:
            // indicateShared(addr, procNum); // Needed for E state
            if (reqType == SNOOPINV) {
                sendData(addr, procNum); 
                *ca = INVALIDATE;
                return INVALID;
            } else if (reqType == SNOOPDNGRADE) {
                sendData(addr, procNum); 
                return SHARED;
            }
            return MODIFIED;

        case SHARED:
            if (reqType == SNOOPINV) {
                sendData(addr, procNum); // ?? Not sure how invalidator gets data
                *ca = INVALIDATE;
                return INVALID;
            } 
            return SHARED;

        case INVALID:
            return INVALID;

        // Stall states; hopefully we receive an update
        case SHARED_MODIFIED:
            if (reqType == GRANTEXCL)
            {
                *ca = DATA_RECV;
                return MODIFIED;
            }
            return SHARED_MODIFIED;

        case INVALID_MODIFIED:
            if (reqType == GRANTEXCL)
            {
                *ca = DATA_RECV;
                return MODIFIED;
            }
            return INVALID_MODIFIED;
        
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
cacheMI(uint8_t is_read, uint8_t* permAvail, coherence_states currentState,
        uint64_t addr, int procNum)
{
    switch (currentState)
    {
        case INVALID:
            *permAvail = 0;
            sendBusWr(addr, procNum);
            return INVALID_MODIFIED;
        case MODIFIED:
            *permAvail = 1;
            return MODIFIED;
        case INVALID_MODIFIED:
            fprintf(stderr, "IM state on %lx, but request %d\n", addr,
                    is_read);
            *permAvail = 0;
            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}

coherence_states
snoopMI(bus_req_type reqType, cache_action* ca, coherence_states currentState,
        uint64_t addr, int procNum)
{
    *ca = NO_ACTION;
    switch (currentState)
    {
        case INVALID:
            return INVALID;
        case MODIFIED:
            sendData(addr, procNum);
            // indicateShared(addr, procNum); // Needed for E state
            *ca = INVALIDATE;
            return INVALID;
        case INVALID_MODIFIED:
            if (reqType == DATA || reqType == SHARED)
            {
                *ca = DATA_RECV;
                return MODIFIED;
            }

            return INVALID_MODIFIED;
        default:
            fprintf(stderr, "State %d not supported, found on %lx\n",
                    currentState, addr);
            break;
    }

    return INVALID;
}