/*
 * def_reaper.c - DEFENCE reaper: finished processes give back their slot.
 *
 * Stops:  zombie_leak (security/zombie_leak)
 * Called: scheduler_tick() (core/scheduler) on every timer tick, and by
 *         `seclab repair`.
 * Rules:  1. only zombies are freed
 *         2. never the running process: we are standing on its stack
 * Why:    a process cannot free its own stack while it runs on it, so
 *         process_exit() only marks it ZOMBIE. Someone else must free it
 *         later, from a different stack. That someone is the reaper.
 */

#include "modules/process_management/security/zombie_leak/defence/reaper/def_reaper.h"  // this file
#include "modules/security_management/core/registry/registry.h"                         // defence_t, registry_add_defence
#include "modules/process_management/core/scheduler/scheduler.h"                        // scheduler_get_current_process
#include "modules/process_management/core/process/process.h"                            // process_t, process_get_table
#include "modules/memory_management/core/kheap/kheap.h"                                 // kfree
#include "arch/x86/cpu/cpu.h"                                                           // cpu_irq_save, cpu_irq_restore
#include "modules/io_management/security/defence/fd_cleanup/def_fd_cleanup.h"           // def_fd_close_all
#include "modules/process_management/core/exitlog/exitlog.h"  // exitlog_add
#include "modules/process_management/core/signal/signal.h"                              // sigqueue_release
#include "modules/cpu_management/core/task/cpu_task.h"                                  // cpu_task_exit

// The whole rule, as a pure function so the selftest can try it.
static int reapable(const process_t *p, const process_t *running) {
    return p && p->state == PROCESS_STATE_ZOMBIE && p != running;
}

void def_reap_zombies(void) {
    // Interrupts off: a timer tick must not reap the same slot twice
    // while we are halfway through freeing it.
    uint32_t flags = cpu_irq_save();

    process_t *table   = process_get_table();
    process_t *running = scheduler_get_current_process();

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (!reapable(p, running))
            continue;

        // DEFENCE fd_cleanup (io_management): close files it left open.
        def_fd_close_all(p);

        // cpu_management: add what it used to its parent, keep a record
        // (`time` and `perf stat` read it).
        cpu_task_exit(p);

        // core/signal: queued signals for it can never be delivered now.
        sigqueue_release(p);

        // core/exitlog: keep its exit status until its parent wait()s.
        exitlog_add(p);

        if (p->stack_base)
            kfree((void *)p->stack_base);
        p->stack_base = 0;
        p->state = PROCESS_STATE_FREE;
    }

    cpu_irq_restore(flags);
}

static int selftest(void) {
    process_t a = {0}, b = {0};

    a.state = PROCESS_STATE_RUNNING;
    if (reapable(&a, &b) != 0) return 0;    // alive: keep

    a.state = PROCESS_STATE_SLEEPING;
    if (reapable(&a, &b) != 0) return 0;    // sleeping is not dead

    a.state = PROCESS_STATE_ZOMBIE;
    if (reapable(&a, &b) != 1) return 0;    // someone else's zombie: free
    if (reapable(&a, &a) != 0) return 0;    // our own: never
    if (reapable(0, &b) != 0) return 0;     // no slot
    return 1;
}

static const defence_t defence = {
    "reaper",
    "free zombie slots and stacks on every timer tick",
    "zombie_leak",
    selftest
};

void def_reaper_register(void) {
    registry_add_defence(&defence);
}
