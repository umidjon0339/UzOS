#ifndef SYS_TIME_H
#define SYS_TIME_H

/*
 * sys_time.h - time(): the date and time as one number.
 */

#include <stdint.h>

#define SYS_TIME 19     // int 0x80 with EAX = 19; seconds since 1970 come back in EAX

// Seconds since 1970-01-01 00:00:00 (Unix time), or 0 if the clock is unreadable.
uint32_t sys_time(void);

#endif
