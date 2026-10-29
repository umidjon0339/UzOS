#ifndef SYS_WAITPID_H
#define SYS_WAITPID_H
/* sys_waitpid.h - waitpid(pid): block until child 'pid' finishes. */
#include <stdint.h>
#define SYS_WAITPID 63      // EAX=63, EBX=pid
int sys_waitpid(uint32_t pid);
#endif
