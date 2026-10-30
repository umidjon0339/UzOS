#ifndef SYS_PSELECT6_H
#define SYS_PSELECT6_H
/* sys_pselect6.h - pselect6(nfds, read, write, except, timeout, sigdata): select() with a timespec and a signal mask. */
#include <stdint.h>
#include "modules/network_management/syscalls/select/sys_select.h"     // select_set_t
#define SYS_PSELECT6 153    // EAX=153, EBX=nfds, ECX/EDX/ESI=sets, EDI=timeout (timespec*), EBP=sigdata*

typedef struct {
    uint32_t sec, nsec;     // struct timespec
} pselect_timespec_t;

// The 6th argument: where the mask is, and how long it is (Linux packs them so).
typedef struct {
    const uint32_t *mask;
    uint32_t size;
} pselect_sigdata_t;

int sys_pselect6(int nfds, select_set_t *rd, select_set_t *wr, select_set_t *ex,
                 const pselect_timespec_t *timeout, const pselect_sigdata_t *sigdata);
#endif
