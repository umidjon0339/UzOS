#ifndef SYS_NANOSLEEP_H
#define SYS_NANOSLEEP_H
/* sys_nanosleep.h - nanosleep(req, rem): sleep for a timespec; a signal may cut it short. */
#include <stdint.h>
#define SYS_NANOSLEEP 86        // EAX=86, EBX=req, ECX=rem (or 0)

// struct timespec.
typedef struct {
    int32_t sec;
    int32_t nsec;               // 0 .. 999999999
} nanosleep_timespec_t;

int sys_nanosleep(const nanosleep_timespec_t *req, nanosleep_timespec_t *rem);
#endif
