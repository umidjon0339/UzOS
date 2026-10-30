#ifndef SYS_PIDFD_GETFD_H
#define SYS_PIDFD_GETFD_H
/* sys_pidfd_getfd.h - pidfd_getfd(pidfd, targetfd, flags): copy another process's open file. */
#include <stdint.h>
#define SYS_PIDFD_GETFD 85      // EAX=85, EBX=pidfd, ECX=targetfd, EDX=flags (0)
int sys_pidfd_getfd(int pidfd, int targetfd, uint32_t flags);
#endif
