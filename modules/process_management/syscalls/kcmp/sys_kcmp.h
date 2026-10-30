#ifndef SYS_KCMP_H
#define SYS_KCMP_H
/* sys_kcmp.h - kcmp(pid1, pid2, type, idx1, idx2): do two processes share a kernel object? */
#include <stdint.h>
#define SYS_KCMP 96             // EAX=96, EBX=pid1, ECX=pid2, EDX=type, ESI=idx1, EDI=idx2

#define KCMP_FILE    0          // file idx1 of pid1 vs file idx2 of pid2
#define KCMP_VM      1          // address space
#define KCMP_FILES   2          // descriptor table
#define KCMP_FS      3          // current directory / root
#define KCMP_SIGHAND 4          // signal handlers
#define KCMP_IO      5
#define KCMP_SYSVSEM 6

int sys_kcmp(uint32_t pid1, uint32_t pid2, int type, uint32_t idx1, uint32_t idx2);
#endif
