#ifndef SYS_KILL_H
#define SYS_KILL_H
/* sys_kill.h - kill(pid, sig): send signal 'sig' to a process or a process group. */
#include <stdint.h>
#define SYS_KILL 58         // EAX=58, EBX=pid, ECX=sig
// pid > 0: that process   0: my process group   -1: every process I may signal
// pid < -1: process group -pid.   sig 0: only check that it exists and I may.
int sys_kill(int32_t pid, int sig);
#endif
