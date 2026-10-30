/*
 * sys_clone3.c - clone3(): the extensible clone().
 *
 * clone() ran out of registers and flag bits. clone3() takes ONE
 * pointer to a struct plus its size; new fields can be added at the
 * end, and the size tells the kernel which version the program knows.
 *
 * Two things differ from clone():
 *   - the stack is given as (lowest address, size), not as its top
 *   - the exit signal has its own field instead of the flags' low byte
 *
 * The child is made exactly as clone() makes it (this file has its own
 * copy of that code): a new process starts in ring 0 at thread_start()
 * and "returns" to ring 3 with a copy of the parent's registers,
 * EAX = 0 and ESP = the top of the new stack (clone3_enter.s).
 *
 * Uses: process_create / process_get_by_pid / the process table and the
 *       current process (process_management), vfs_open
 *       (filesystem_management), cpu_irq_save (arch/x86/cpu),
 *       def_thread_quota_ok (process_management defence thread_quota),
 *       clone3_enter_user (clone3_enter.s, this folder).
 */

#include "modules/cpu_management/syscalls/clone3/sys_clone3.h"                                            // this syscall, CLONE_*
#include "modules/process_management/core/scheduler/scheduler.h"                                          // the current process
#include "modules/filesystem_management/core/vfs/vfs.h"                                                   // vfs_open
#include "arch/x86/cpu/cpu.h"                                                                             // cpu_irq_save
#include "modules/process_management/security/thread_exhaustion/defence/thread_quota/def_thread_quota.h"  // def_thread_quota_ok

#define SUPPORTED (CLONE_VM | CLONE_SIGHAND | CLONE_THREAD | \
                   CLONE_PARENT_SETTID | CLONE_CHILD_SETTID)

// clone3_enter.s
extern void clone3_enter_user(const registers_t *frame) __attribute__((noreturn));

// The registers each new thread starts with, by process slot.
static registers_t start_frames[MAX_PROCESSES];

static int slot_of(const process_t *p) {
    return p - process_get_table();
}

// The first code of every new thread (in ring 0, on its own stack).
static void thread_start(void) {
    process_t *self = scheduler_get_current_process();
    clone3_enter_user(&start_frames[slot_of(self)]);
}

// The struct's rules, then Linux's rules for the flags (CLONE_VM required).
static int args_ok(const clone_args_t *a, uint32_t size) {
    if (!a || size < CLONE_ARGS_SIZE_VER0)
        return 0;
    if (a->flags > 0xFFFFFFFFull || (a->flags & ~(uint64_t)SUPPORTED))
        return 0;                                   // unknown, CSIGNAL bits, CLONE_FS, ...
    if (a->pidfd || a->tls || a->stack_size == 0)
        return 0;                                   // not in UzOS / no stack given
    if (!(a->flags & CLONE_VM))
        return 0;                                   // no copy of memory exists
    if ((a->flags & CLONE_THREAD) && !(a->flags & CLONE_SIGHAND))
        return 0;                                   // Linux: a thread shares handlers
    return 1;
}

int sys_clone3(const registers_t *regs, const clone_args_t *args, uint32_t size) {
    process_t *parent = scheduler_get_current_process();

    if ((regs->cs & 3) != 3 || !parent || !args_ok(args, size))
        return -1;                                  // only ring 3 programs clone

    uint32_t flags = (uint32_t)args->flags;
    uint32_t stack_top = (uint32_t)(args->stack + args->stack_size);
    uint32_t *parent_tid = (uint32_t *)(uint32_t)args->parent_tid;
    uint32_t *child_tid  = (uint32_t *)(uint32_t)args->child_tid;

    // DEFENCE thread_quota (process_management): a program has at most 4 threads.
    if ((flags & CLONE_THREAD) && !def_thread_quota_ok(parent))
        return -1;

    // Interrupts off: the child must not run before its frame is ready.
    uint32_t irq = cpu_irq_save();

    int pid = process_create(parent->name, thread_start, parent->uid);
    process_t *child = pid > 0 ? process_get_by_pid(pid) : 0;
    if (!child) {
        cpu_irq_restore(irq);
        return -1;
    }

    // The registers it "returns" with: the parent's, except two.
    registers_t *frame = &start_frames[slot_of(child)];
    *frame = *regs;
    frame->eax     = 0;                             // "you are the child"
    frame->useresp = stack_top;                     // its own stack

    if (flags & CLONE_THREAD) {
        child->tgid = parent->tgid;                 // same process for getpid()
        child->ppid = parent->ppid;                 // a thread's parent is the group's
        child->membarrier_registered = parent->membarrier_registered;
    }

    // Current directory and open files: copies, as after fork().
    child->cwd = parent->cwd;
    for (int fd = 0; fd < MAX_FDS; fd++) {
        child->fd_table[fd]  = parent->fd_table[fd];
        child->fd_offset[fd] = parent->fd_offset[fd];
        child->fd_flags[fd]  = parent->fd_flags[fd];
        if (child->fd_table[fd])
            vfs_open(child->fd_table[fd]);          // one more descriptor points there
    }

    if ((flags & CLONE_PARENT_SETTID) && parent_tid)
        *parent_tid = pid;
    if ((flags & CLONE_CHILD_SETTID) && child_tid)
        *child_tid = pid;                           // same memory: the child sees it

    cpu_irq_restore(irq);
    return pid;
}
