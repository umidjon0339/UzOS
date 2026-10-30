/*
 * sys_vfork.c - vfork(): start a child that runs in my memory, on my
 * stack, while I wait.
 *
 *     pid = vfork();
 *     if (pid == 0) { execve("/prog", argv, envp); _exit(127); }
 *     // here: the child has exec'd (or ended); pid is its pid
 *
 * fork() would COPY the parent's memory; UzOS has one address space and
 * cannot copy it (see sys_fork). vfork() never needed a copy: the child
 * borrows everything, and the parent is suspended so the two never use
 * the stack at the same time. That is exactly the old Unix contract, and
 * vfork + execve is how a program starts another one in UzOS.
 *
 * The child is a new process (pid, a kernel stack, a copy of the open
 * files) that leaves the kernel through vfork_enter_user with the
 * parent's registers and EAX = 0. Its vfork_parent field holds the
 * parent until execve() clears it, or the child ends.
 *
 * Uses: process_create / process_get_by_pid (core/process),
 *       scheduler_get_current_process (core/scheduler), vfs_open
 *       (filesystem_management), cpu_irq_save, sys_sleep (syscalls/sleep),
 *       vfork_enter_user (vfork_enter.s, this folder).
 */
#include "modules/process_management/syscalls/vfork/sys_vfork.h"       // this file
#include "modules/process_management/core/scheduler/scheduler.h"       // scheduler_get_current_process
#include "modules/process_management/syscalls/sleep/sys_sleep.h"       // sys_sleep
#include "modules/filesystem_management/core/vfs/vfs.h"                // vfs_open
#include "arch/x86/cpu/cpu.h"                                          // cpu_irq_save, cpu_irq_restore

#define WAIT_STEP_MS 5

// vfork_enter.s
extern void vfork_enter_user(const registers_t *frame) __attribute__((noreturn));

// The registers each new child starts with, by process slot.
static registers_t start_frames[MAX_PROCESSES];

static int slot_of(const process_t *p) {
    return p - process_get_table();
}

static void child_start(void) {
    vfork_enter_user(&start_frames[slot_of(scheduler_get_current_process())]);
}

int sys_vfork(const registers_t *regs) {
    process_t *parent = scheduler_get_current_process();
    if (!parent || (regs->cs & 3) != 3)
        return -1;                                  // only ring 3 programs

    uint32_t irq = cpu_irq_save();                  // it must not run half set up
    int pid = process_create(parent->name, child_start, parent->uid);
    process_t *child = pid > 0 ? process_get_by_pid(pid) : 0;
    if (!child) {
        cpu_irq_restore(irq);
        return -1;
    }
    start_frames[slot_of(child)] = *regs;
    start_frames[slot_of(child)].eax = 0;           // "you are the child"
    child->cwd = parent->cwd;
    for (int fd = 0; fd < MAX_FDS; fd++) {          // a copy of the descriptor table
        child->fd_table[fd]  = parent->fd_table[fd];
        child->fd_offset[fd] = parent->fd_offset[fd];
        child->fd_flags[fd]  = parent->fd_flags[fd];
        if (child->fd_table[fd])
            vfs_open(child->fd_table[fd]);
    }
    child->vfork_parent = parent->pid;
    cpu_irq_restore(irq);

    // Wait while the child uses our stack: until it exec'd or ended.
    for (process_t *c; (c = process_get_by_pid(pid)) && c->state != PROCESS_STATE_ZOMBIE &&
                       c->vfork_parent == parent->pid; )
        sys_sleep(WAIT_STEP_MS);
    return pid;
}
