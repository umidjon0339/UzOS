#ifndef SYS_ALARM_H
#define SYS_ALARM_H
/* sys_alarm.h - alarm(seconds): SIGALRM after this many seconds. */
#include <stdint.h>
#define SYS_ALARM 79            // EAX=79, EBX=seconds (0 = cancel)
uint32_t sys_alarm(uint32_t seconds);
#endif
