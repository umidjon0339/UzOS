#ifndef SYS_CLONE3_H
#define SYS_CLONE3_H

/*
 * sys_clone3.h - clone3(): clone() with its arguments in a struct.
 *
 * The same thread as clone() makes (see clone/sys_clone.h for the
 * flags); this folder has its own copy of the code, so it can be read
 * on its own.
 */

#include <stdint.h>
#include "arch/x86/isr/isr.h"                               // registers_t

#define SYS_CLONE3 44   // EAX = 44, EBX = &clone_args, ECX = sizeof(clone_args)

// The flags clone3 takes (Linux values; the same as clone's).
#define CLONE_VM             0x00000100
#define CLONE_SIGHAND        0x00000800
#define CLONE_THREAD         0x00010000
#define CLONE_PARENT_SETTID  0x00100000
#define CLONE_CHILD_SETTID   0x01000000
#define CSIGNAL              0x000000FF     // low byte: signal to the parent at exit (ignored)

// Linux's struct clone_args (first version, 64 bytes): every field is
// 64-bit, so the same struct works for 32- and 64-bit programs.
typedef struct {
    uint64_t flags;         // CLONE_*
    uint64_t pidfd;         // CLONE_PIDFD: not in UzOS
    uint64_t child_tid;     // where CLONE_CHILD_SETTID writes
    uint64_t parent_tid;    // where CLONE_PARENT_SETTID writes
    uint64_t exit_signal;   // signal to the parent at exit (ignored)
    uint64_t stack;         // LOWEST address of the new stack...
    uint64_t stack_size;    // ...and its size: the top is stack + size
    uint64_t tls;           // thread-local storage: not in UzOS
} clone_args_t;
#define CLONE_ARGS_SIZE_VER0 64

// Like clone(). 'regs' are the caller's saved registers.
int sys_clone3(const registers_t *regs, const clone_args_t *args, uint32_t size);

#endif
