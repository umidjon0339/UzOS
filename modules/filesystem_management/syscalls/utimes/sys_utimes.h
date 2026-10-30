#ifndef SYS_UTIMES_H
#define SYS_UTIMES_H
/* sys_utimes.h - utimes(path, tv[2]): set access and change times, in microseconds. */
#include <stdint.h>
#define SYS_UTIMES 117      // EAX=117, EBX=path, ECX=tv (2 x struct timeval, or 0 = now)
typedef struct {
    uint32_t sec;
    uint32_t usec;          // UzOS keeps whole seconds: dropped
} utimes_timeval_t;
int sys_utimes(const char *path, const utimes_timeval_t tv[2]);
#endif
