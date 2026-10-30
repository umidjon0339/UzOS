#ifndef SYS_GETITIMER_H
#define SYS_GETITIMER_H
/* sys_getitimer.h - getitimer(which, cur): how long until a timer fires. */
#include <stdint.h>
#define SYS_GETITIMER 89        // EAX=89, EBX=which, ECX=cur

// struct itimerval.
typedef struct {
    int32_t interval_sec, interval_usec;
    int32_t value_sec, value_usec;          // time left (0 = not armed)
} getitimer_val_t;

int sys_getitimer(int which, getitimer_val_t *cur);
#endif
