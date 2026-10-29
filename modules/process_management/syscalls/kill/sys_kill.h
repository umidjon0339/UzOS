#ifndef SYS_KILL_H
#define SYS_KILL_H
/* sys_kill.h - kill(pid, sig): send signal 'sig' to process 'pid'. */
#include <stdint.h>
#define SYS_KILL 58         // EAX=58, EBX=pid, ECX=sig
int sys_kill(uint32_t pid, int sig);
#endif
