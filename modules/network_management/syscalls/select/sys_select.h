#ifndef SYS_SELECT_H
#define SYS_SELECT_H
/*
 * sys_select.h - select(nfds, read, write, except, timeout): the oldest way to wait.
 *
 * The descriptor sets are bitmaps: bit n set = "descriptor n". On
 * return only the bits of ready descriptors are still set.
 */
#include <stdint.h>
#define SYS_SELECT 152      // EAX=152, EBX=nfds, ECX=read set, EDX=write set, ESI=except set, EDI=timeout (timeval*, 0 = forever)
#define SELECT_FDS 32       // UzOS: one 32-bit word per set (MAX_FDS is 16)

typedef struct {
    uint32_t bits;          // Linux's fd_set has 1024 bits; descriptors here are below 32
} select_set_t;

typedef struct {
    uint32_t sec, usec;     // struct timeval
} select_timeval_t;

// How many bits are set in all three sets now, 0 on timeout, or -errno.
int sys_select(int nfds, select_set_t *rd, select_set_t *wr, select_set_t *ex,
               const select_timeval_t *timeout);

// The work, shared with pselect6: the timeout in milliseconds (-1 = forever).
int select_wait(int nfds, select_set_t *rd, select_set_t *wr, select_set_t *ex, int timeout_ms);
#endif
