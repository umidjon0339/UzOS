#ifndef SYS_CPUTIME_H
#define SYS_CPUTIME_H

/*
 * sys_cputime.h - cputime(): how much CPU time a process has used.
 */

#include <stdint.h>

#define SYS_CPUTIME 20  // int 0x80 with EAX = 20, EBX = pid (0 = me); ms back in EAX

// CPU milliseconds used by 'pid' (0 = the caller), or -1 if no such process.
int32_t sys_cputime(uint32_t pid);

#endif
