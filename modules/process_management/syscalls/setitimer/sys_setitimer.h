#ifndef SYS_SETITIMER_H
#define SYS_SETITIMER_H
/* sys_setitimer.h - setitimer(which, new, old): arm an interval timer. */
#include <stdint.h>
#define SYS_SETITIMER 88        // EAX=88, EBX=which, ECX=new, EDX=old (or 0)

// struct itimerval: first fire after 'value', then every 'interval'.
typedef struct {
    int32_t interval_sec, interval_usec;    // 0 = fire once
    int32_t value_sec, value_usec;          // 0 = disarm
} setitimer_val_t;

int sys_setitimer(int which, const setitimer_val_t *new_value, setitimer_val_t *old_value);
#endif
