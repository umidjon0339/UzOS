#ifndef SYS_TKILL_H
#define SYS_TKILL_H
/* sys_tkill.h - tkill(tid, sig): signal one thread (a process in UzOS). */
#include <stdint.h>
#define SYS_TKILL 59        // EAX=59, EBX=tid, ECX=sig
int sys_tkill(uint32_t tid, int sig);
#endif
