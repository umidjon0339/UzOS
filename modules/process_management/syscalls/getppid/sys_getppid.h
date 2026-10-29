#ifndef SYS_GETPPID_H
#define SYS_GETPPID_H
/* sys_getppid.h - getppid(): the parent process's id. */
#include <stdint.h>
#define SYS_GETPPID 52      // int 0x80 with EAX = 52
uint32_t sys_getppid(void);
#endif
