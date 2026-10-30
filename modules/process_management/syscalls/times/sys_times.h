#ifndef SYS_TIMES_H
#define SYS_TIMES_H
/* sys_times.h - times(buf): CPU times in clock ticks, and the ticks since boot. */
#include <stdint.h>
#define SYS_TIMES 83            // EAX=83, EBX=buf (or 0)

// struct tms: clock ticks (100 a second, sysconf(_SC_CLK_TCK)).
typedef struct {
    uint32_t utime;             // mine, ring 3
    uint32_t stime;             // mine, ring 0
    uint32_t cutime;            // my finished children, ring 3
    uint32_t cstime;            // my finished children, ring 0
} tms_t;

uint32_t sys_times(tms_t *buf);
#endif
