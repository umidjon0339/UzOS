#ifndef SYS_SETPGID_H
#define SYS_SETPGID_H
/* sys_setpgid.h - setpgid(pid, pgid): put a process in a process group. */
#include <stdint.h>
#define SYS_SETPGID 53      // EAX=53, EBX=pid (0=self), ECX=pgid (0=pid)
int sys_setpgid(uint32_t pid, uint32_t pgid);
#endif
