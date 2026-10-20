#ifndef SYS_UPTIME_H
#define SYS_UPTIME_H

/*
 * sys_uptime.h - uptime(): milliseconds since the system started.
 */

#include <stdint.h>

#define SYS_UPTIME 8    // int 0x80 with EAX = 8; milliseconds come back in EAX

uint32_t sys_uptime(void);

#endif
