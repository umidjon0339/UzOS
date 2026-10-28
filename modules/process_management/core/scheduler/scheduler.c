/*
 * scheduler.c - Pick the next process and switch to it.
 *
 * This file is the MECHANISM: it keeps track of who runs and swaps the
 * CPU over. The POLICY (which process next, for how long) belongs to
 * cpu_management/core/priority.
 *
 * Uses: switch_task (switch_task.s) to swap stacks, set_kernel_stack
 *       (arch/x86/tss) so ring 3 interrupts land on the right stack,
 *       cpu_irq_save / cpu_read_esp (arch/x86/cpu), priority_pick_next /
 *       priority_tick / priority_start_turn (cpu_management), cpu_account_switch
 *       (cpu_management: times every switch), power_idle (cpu_management:
 *       what the idle CPU does), and two defences:
 *         stack_guard - retire a process whose stack overflowed (yield)
 *         reaper      - free the slots of finished processes (tick)
 */

#include "scheduler.h"
#include "process.h"
#include "tss.h"
#include "cpu.h"
#include "modules/cpu_management/core/priority/priority.h"        // priority_*
#include "modules/cpu_management/core/accounting/cpu_account.h"   // cpu_account_switch
#include "modules/cpu_management/core/power/power.h"              // power_idle

#include "def_stack_guard.h"
#include "def_reaper.h"

#define EFLAGS_RESERVED 0x2     // bit 1 of EFLAGS is always 1

extern void switch_task(uint32_t *save_esp, uint32_t load_esp);

static process_t *current;      // the process on the CPU
static process_t *idle;         // the idle task
static int started;             // 0 until scheduler_start()

// Runs when nothing else is READY, so the scheduler always has a choice.
// What "doing nothing" means (hlt, or spinning) is the power profile's
// choice (cpu_management/core/power).
static void idle_task(void) {
    while (1)
        power_idle();
}

static process_t *mark_system(int pid) {
    process_t *p = process_get_by_pid(pid);
    if (p)
        p->is_system = 1;       // kill_guard refuses to kill these
    return p;
}

void scheduler_init(void) {
    process_init();

    idle = mark_system(process_create("idle", idle_task, 0));
    idle->priority = PRIORITY_IDLE;     // runs only when nobody else can

    // kernel_main is already running on the boot stack. Giving it a slot
    // makes it a normal process the scheduler can switch away from and back.
    current = mark_system(process_create("kernel_main", 0, 0));
    current->state = PROCESS_STATE_RUNNING;
    priority_start_turn(current);
}

void scheduler_start(void) {
    started = 1;
}

process_t *scheduler_get_current_process(void) {
    return current;
}

process_t *scheduler_idle_process(void) {
    return idle;
}

uint32_t scheduler_prepare_stack(uint32_t stack_top, void (*first)(void)) {
    uint32_t *sp = (uint32_t *)stack_top;

    // The same layout switch_task leaves behind (see switch_task.s).
    *--sp = (uint32_t)first;                    // 'ret' jumps here
    *--sp = EFLAGS_IF | EFLAGS_RESERVED;        // start with interrupts on
    *--sp = 0;                                  // ebp
    *--sp = 0;                                  // ebx
    *--sp = 0;                                  // esi
    *--sp = 0;                                  // edi

    return (uint32_t)sp;
}

// Count down sleeping processes; wake the ones whose time is up.
static void wake_sleepers(void) {
    process_t *table = process_get_table();

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state != PROCESS_STATE_SLEEPING)
            continue;
        if (p->sleep_ticks > 0)
            p->sleep_ticks--;
        if (p->sleep_ticks == 0)
            p->state = PROCESS_STATE_READY;
    }
}

void scheduler_tick(void) {
    wake_sleepers();            // even before start, so early sleeps end

    if (!started)
        return;

    // DEFENCE reaper: free finished processes. Safe because the reaper
    // never touches the current process, whose stack we are on.
    def_reap_zombies();

    // Is this turn over, or is someone more important waiting?
    if (priority_tick(current))
        scheduler_yield();
}

void scheduler_yield(void) {
    if (!started)
        return;

    // A switch must not be interrupted halfway: between "current = next"
    // and the actual stack swap, the tables and the CPU disagree.
    uint32_t flags = cpu_irq_save();

    // DEFENCE stack_guard: we are on the current process's stack right
    // now. If ESP has left it, the process overflowed: retire it.
    if (!def_stack_guard_check(current, cpu_read_esp()))
        current->state = PROCESS_STATE_ZOMBIE;

    process_t *next = priority_pick_next(current);
    if (next == current) {
        priority_start_turn(current);   // nobody better: keep going, new slice
    } else if (next) {
        process_t *prev = current;

        cpu_account_switch(prev, next);     // first: still RUNNING = it was preempted
        if (prev->state == PROCESS_STATE_RUNNING)
            prev->state = PROCESS_STATE_READY;
        next->state = PROCESS_STATE_RUNNING;
        priority_start_turn(next);
        current = next;

        // An interrupt from ring 3 must land on this process's stack.
        // (kernel_main has no stack of its own and never enters ring 3.)
        if (next->stack_base)
            set_kernel_stack(next->stack_base + PROCESS_STACK_SIZE);

        switch_task(&prev->esp, next->esp);
        // ...some time later another switch_task resumes us right here.
    }

    cpu_irq_restore(flags);
}
