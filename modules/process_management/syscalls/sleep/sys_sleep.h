#ifndef SYS_SLEEP_H
#define SYS_SLEEP_H

/*
 * sys_sleep.h - sleep(): pause the calling process for a while.
 */

#include <stdint.h>

#define SYS_SLEEP 3     // int 0x80 with EAX = 3, EBX = milliseconds

void sys_sleep(uint32_t ms);

#endif
