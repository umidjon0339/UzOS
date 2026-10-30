#ifndef SYS_PIDFD_OPEN_H
#define SYS_PIDFD_OPEN_H
/* sys_pidfd_open.h - pidfd_open(pid, flags): a file descriptor that names one process. */
#include <stdint.h>
#define SYS_PIDFD_OPEN 84       // EAX=84, EBX=pid, ECX=flags (0)
int sys_pidfd_open(uint32_t pid, uint32_t flags);
#endif
