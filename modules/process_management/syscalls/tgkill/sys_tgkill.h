#ifndef SYS_TGKILL_H
#define SYS_TGKILL_H
/* sys_tgkill.h - tgkill(tgid, tid, sig): signal a thread in a group. */
#include <stdint.h>
#define SYS_TGKILL 60       // EAX=60, EBX=tgid, ECX=tid, EDX=sig
int sys_tgkill(uint32_t tgid, uint32_t tid, int sig);
#endif
