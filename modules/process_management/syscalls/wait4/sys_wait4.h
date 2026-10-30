#ifndef SYS_WAIT4_H
#define SYS_WAIT4_H
/* sys_wait4.h - wait4(pid, wstatus, options, rusage): wait for a child; how it ended, what it used. */
#include <stdint.h>
#define SYS_WAIT4 81            // EAX=81, EBX=pid, ECX=wstatus (or 0), EDX=options, ESI=rusage (or 0)
#define WNOHANG   1             // do not wait: 0 if no child has ended yet

// struct rusage, as Linux on 32-bit x86 lays it out (72 bytes).
typedef struct {
    uint32_t utime_sec, utime_usec;     // CPU time in ring 3
    uint32_t stime_sec, stime_usec;     // CPU time in ring 0
    uint32_t maxrss;                    // KiB (UzOS: its stack)
    uint32_t ixrss, idrss, isrss, minflt, majflt, nswap, inblock, oublock,
             msgsnd, msgrcv, nsignals;  // 0 in UzOS
    uint32_t nvcsw, nivcsw;             // context switches: gave up / preempted
} wait4_rusage_t;

int sys_wait4(int32_t pid, int32_t *wstatus, uint32_t options, wait4_rusage_t *rusage);
#endif
