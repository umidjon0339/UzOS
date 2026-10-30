/*
 * sys_clone.c - clone(): one call, two returns.
 *
 *     tid = clone(CLONE_VM | CLONE_SIGHAND | CLONE_THREAD, stack_top, ...);
 *     if (tid == 0)   -> I am the new thread, on stack_top
 *     else            -> I am the parent, tid is the new thread
 *
 * How the child gets there:
 *
 *   parent (ring 3)                        child
 *   int 0x80 (clone)
 *     copy the parent's saved registers, change two:
 *         eax = 0 (so the child knows it is the child)
 *         esp = 'stack'
 *     process_create(..., thread_start)
 *   parent returns with eax = child id     thread_start() in ring 0
 *                                          clone_enter_user(copy) -> iret
 *                                          -> ring 3, after int 0x80, eax 0
 *
 * A thread is a normal UzOS process (a slot, a pid, a kernel stack); it
 * only shares the memory, which every process does in UzOS today.
 *
 * Uses: process_create / process_get_by_pid / the process table and the
 *       current process (process_management), vfs_open
 *       (filesystem_management) for the copied files, cpu_irq_save
 *       (arch/x86/cpu), def_thread_quota_ok (process_management defence
 *       thread_quota), clone_enter_user (clone_enter.s, this folder).
 */

#include "modules/cpu_management/syscalls/clone/sys_clone.h"                                              // this syscall, CLONE_*
#include "modules/process_management/core/scheduler/scheduler.h"                                          // the current process
#include "modules/filesystem_management/core/vfs/vfs.h"                                                   // vfs_open
#include "arch/x86/cpu/cpu.h"                                                                             // cpu_irq_save
#include "modules/process_management/security/thread_exhaustion/defence/thread_quota/def_thread_quota.h"  // def_thread_quota_ok

#define SUPPORTED (CLONE_VM | CLONE_SIGHAND | CLONE_THREAD | \
                   CLONE_PARENT_SETTID | CLONE_CHILD_SETTID | CSIGNAL)

// clone_enter.s
extern void clone_enter_user(const registers_t *frame) __attribute__((noreturn));

// The registers each new thread starts with, by process slot.
static registers_t start_frames[MAX_PROCESSES];

static int slot_of(const process_t *p) {
    return p - process_get_table();
}

// The first code of every new thread (in ring 0, on its own stack).
static void thread_start(void) {
    process_t *self = scheduler_get_current_process();
    clone_enter_user(&start_frames[slot_of(self)]);
}

// Linux's rules for the flags, plus UzOS's own (CLONE_VM required).
static int flags_ok(uint32_t flags) {
    if (flags & ~SUPPORTED)
        return 0;                                   // CLONE_FS, CLONE_FILES, ...
    if (!(flags & CLONE_VM))
        return 0;                                   // no copy of memory exists
    if ((flags & CLONE_THREAD) && !(flags & CLONE_SIGHAND))
        return 0;                                   // Linux: a thread shares handlers
    return 1;
}

int sys_clone(const registers_t *regs, uint32_t flags, uint32_t stack,
              uint32_t *parent_tid, uint32_t *child_tid) {
    process_t *parent = scheduler_get_current_process();

    if ((regs->cs & 3) != 3 || !parent)
        return -1;                                  // only ring 3 programs clone
    if (!flags_ok(flags) || stack == 0)
        return -1;                                  // shared memory needs its own stack
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
    frame->useresp = stack;                         // its own stack

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
