#ifndef SYS_CLOCK_NANOSLEEP_H
#define SYS_CLOCK_NANOSLEEP_H
/* sys_clock_nanosleep.h - clock_nanosleep(clock, flags, req, rem): sleep on a chosen clock. */
#include <stdint.h>
#define SYS_CLOCK_NANOSLEEP 87  // EAX=87, EBX=clock, ECX=flags, EDX=req, ESI=rem (or 0)
#define CLOCK_REALTIME  0       // the date and time (the RTC)
#define CLOCK_MONOTONIC 1       // time since boot: never jumps
#define TIMER_ABSTIME   1       // req is a moment on that clock, not a length

// Linux returns the error number itself (not -1).
#define CNS_EINTR  4
#define CNS_EINVAL 22

typedef struct {
    int32_t sec;
    int32_t nsec;
} clock_timespec_t;

int sys_clock_nanosleep(int clock, int flags, const clock_timespec_t *req, clock_timespec_t *rem);
#endif
