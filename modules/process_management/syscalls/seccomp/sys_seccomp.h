#ifndef SYS_SECCOMP_H
#define SYS_SECCOMP_H
/* sys_seccomp.h - seccomp(op, flags, args): give up syscalls for good. */
#include <stdint.h>
#define SYS_SECCOMP 90          // EAX=90, EBX=op, ECX=flags (0), EDX=args

#define SECCOMP_SET_MODE_STRICT  0  // args 0: only read, write, exit, rt_sigreturn
#define SECCOMP_SET_MODE_FILTER  1  // args: seccomp_list_t
#define SECCOMP_GET_ACTION_AVAIL 2  // args: uint32_t action -> 0 if UzOS has it

// UzOS's filter: Linux runs a small BPF program on every call; UzOS
// takes the answer such a program would give, as a list.
typedef struct {
    uint32_t action;            // for a syscall not in 'allow': SECCOMP_RET_*
    uint32_t allow[4];          // bit n = syscall n is allowed (0..127)
} seccomp_list_t;

int sys_seccomp(uint32_t op, uint32_t flags, void *args);
#endif
