#ifndef DEF_SECCOMP_FILTER_H
#define DEF_SECCOMP_FILTER_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

#define SECCOMP_MODE_STRICT 1
#define SECCOMP_MODE_FILTER 2

// Actions (the same values as Linux's SECCOMP_RET_*).
#define SECCOMP_RET_KILL_THREAD  0x00000000u    // end the program (SIGSYS)
#define SECCOMP_RET_KILL_PROCESS 0x80000000u
#define SECCOMP_RET_ERRNO        0x00050000u    // the call fails with -1
#define SECCOMP_RET_ALLOW        0x7FFF0000u

// May 'p' make syscall 'nr'? Strict mode: read, write, exit and
// rt_sigreturn only. Filter mode: the bits it allowed. The syscall
// handler asks this BEFORE every call; if 0 (logged), it carries out
// def_seccomp_action(p): end the program, or fail the call.
int def_seccomp_allowed(const process_t *p, uint32_t nr);
uint32_t def_seccomp_action(const process_t *p);

#endif
