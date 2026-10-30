#ifndef SYS_GETRUSAGE_H
#define SYS_GETRUSAGE_H
/* sys_getrusage.h - getrusage(who, usage): CPU used by me, or by my finished children. */
#include <stdint.h>
#define SYS_GETRUSAGE 82        // EAX=82, EBX=who, ECX=usage
#define RUSAGE_SELF      0
#define RUSAGE_CHILDREN (-1)    // the children that ended (and were reaped)
#define RUSAGE_THREAD    1      // this thread only (UzOS: the same as SELF)

// struct rusage, as Linux on 32-bit x86 lays it out (72 bytes).
typedef struct {
    uint32_t utime_sec, utime_usec;
    uint32_t stime_sec, stime_usec;
    uint32_t maxrss;
    uint32_t ixrss, idrss, isrss, minflt, majflt, nswap, inblock, oublock,
             msgsnd, msgrcv, nsignals;
    uint32_t nvcsw, nivcsw;
} rusage_t;

int sys_getrusage(int who, rusage_t *usage);
#endif
