#ifndef SYS_GETPGID_H
#define SYS_GETPGID_H
/* sys_getpgid.h - getpgid(pid): a process's group id (pid 0 = self). */
#include <stdint.h>
#define SYS_GETPGID 54      // EAX=54, EBX=pid (0=self)
int sys_getpgid(uint32_t pid);
#endif
