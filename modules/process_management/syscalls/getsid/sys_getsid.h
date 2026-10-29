#ifndef SYS_GETSID_H
#define SYS_GETSID_H
/* sys_getsid.h - getsid(pid): a process's session id (pid 0 = self). */
#include <stdint.h>
#define SYS_GETSID 57       // EAX=57, EBX=pid (0=self)
int sys_getsid(uint32_t pid);
#endif
