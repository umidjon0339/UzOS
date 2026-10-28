#ifndef SYS_CLONE_H
#define SYS_CLONE_H

/*
 * sys_clone.h - clone(): make a new thread that shares my memory.
 *
 * fork() would give the child a COPY of the parent's memory. UzOS
 * cannot copy an address space yet: there is one page directory for
 * everybody. What it CAN do is the other kind of clone(), the one that
 * makes THREADS: the child runs in the SAME memory, on its own stack.
 *
 * Flags UzOS takes (Linux values):
 *   CLONE_VM             share memory - REQUIRED (nothing else exists)
 *   CLONE_SIGHAND        share signal handlers (UzOS has only defaults)
 *   CLONE_THREAD         same thread group: getpid() gives the parent's,
 *                        exit_group() ends them all
 *   CLONE_PARENT_SETTID  write the child's id to *parent_tid
 *   CLONE_CHILD_SETTID   write the child's id to *child_tid
 * Refused: CLONE_FS, CLONE_FILES (UzOS keeps the current directory and
 * the open files inside each process, so they cannot be shared; the
 * child gets a copy of both, as after fork()), and every other flag.
 */

#include <stdint.h>
#include "arch/x86/isr/isr.h"                               // registers_t

#define SYS_CLONE 43    // EAX = 43, EBX = flags, ECX = new stack, EDX = &parent_tid, ESI = tls (unused), EDI = &child_tid

#define CLONE_VM             0x00000100
#define CLONE_FS             0x00000200
#define CLONE_FILES          0x00000400
#define CLONE_SIGHAND        0x00000800
#define CLONE_THREAD         0x00010000
#define CLONE_PARENT_SETTID  0x00100000
#define CLONE_CHILD_SETTID   0x01000000
#define CSIGNAL              0x000000FF     // low byte: signal to the parent at exit (ignored)

// 'regs' are the caller's saved registers (the child starts as a copy).
// Parent gets the child's id, the child gets 0, -1 on error.
int sys_clone(const registers_t *regs, uint32_t flags, uint32_t stack,
              uint32_t *parent_tid, uint32_t *child_tid);

#endif
