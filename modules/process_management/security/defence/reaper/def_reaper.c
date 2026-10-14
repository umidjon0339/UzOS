#include "proc_security.h"
#include "scheduler.h"
#include "kheap.h"
#include "string.h"

/* ===================================================================
 * DEFENCE: reaper
 * ===================================================================
 * Restores INV_RESOURCE - every slot and stack comes back eventually.
 *
 * A process that has exited is a zombie: it will never run again, but
 * its slot and its 4 KB stack are still held. The original
 * process_exit() simply halted in a loop, so the resources were never
 * returned at all.
 *
 * Two rules make reaping safe:
 *
 *   1. only zombies are reaped;
 *   2. never reap the process that is currently running, because
 *      freeing its stack would pull the floor out from under us.
 *
 * The reaper runs from the timer interrupt, which is exactly why it is
 * a safe place to free memory: by then the CPU is on an interrupt
 * stack, not on the stack being freed. That is the whole trick.
 * =================================================================== */

static int reapable(process_t *p, uint32_t current_pid) {
    if (!p) return 0;
    if (p->state != PROCESS_STATE_ZOMBIE) return 0;
    if (p->pid == current_pid) return 0;   /* that is our own floor */
    return 1;
}

int def_is_reapable(process_t *p) {
    process_t *self = scheduler_get_current_process();
    return reapable(p, self ? self->pid : 0);
}

void def_reap_zombies(void) {
    /* During a context switch current_process already points at the
     * incoming task while the CPU is still running on the outgoing
     * task's stack. Freeing anything in that window would corrupt the
     * switch itself, so do nothing and try again on the next tick. */
    if (scheduler_is_switching()) return;

    process_t *table = process_get_table();
    process_t *self  = scheduler_get_current_process();
    uint32_t current_pid = self ? self->pid : 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (!reapable(p, current_pid)) continue;

        if (p->stack_base) kfree((void *)p->stack_base);

        /* the parent is one child lighter now */
        def_child_release(process_get_by_pid(p->parent_pid));

        p->stack_base = 0;
        p->esp        = 0;
        p->pid        = 0;
        p->state      = PROCESS_STATE_FREE;
    }
}

static int def_reaper_selftest(void) {
    process_t probe;
    memset(&probe, 0, sizeof(probe));

    /* a live process is never reaped */
    probe.state = PROCESS_STATE_RUNNING;
    probe.pid   = 1234;
    if (reapable(&probe, 999)  != 0) return 0;

    /* a zombie belonging to someone else is reaped */
    probe.state = PROCESS_STATE_ZOMBIE;
    if (reapable(&probe, 999)  != 1) return 0;

    /* the current process is never reaped, even as a zombie */
    if (reapable(&probe, 1234) != 0) return 0;

    /* a sleeping process is not a zombie */
    probe.state = PROCESS_STATE_SLEEPING;
    if (reapable(&probe, 999)  != 0) return 0;

    /* a null slot is ignored */
    if (reapable(0, 999) != 0) return 0;

    return 1;
}

static const defence_t def = {
    "reaper",
    "reclaim zombie slots and stacks from the timer interrupt",
    "zombie_leak",
    def_reaper_selftest
};

void def_reaper_register(void) {
    proc_security_register_defence(&def);
}
