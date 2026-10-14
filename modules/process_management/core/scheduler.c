#include "scheduler.h"
#include "process.h"
#include "gdt.h"
#include "proc_security.h"
#include <stddef.h>

/* ===================================================================
 * Round-robin scheduler.
 *
 * Two defences live here:
 *   stack_guard - refuses to switch into a corrupted stack
 *   reaper      - reclaims zombie slots from a safe stack
 * =================================================================== */

static process_t *current_process = NULL;
static int current_process_index = -1;
static int scheduler_enabled = 0;

/* Set while a context switch is in progress.
 *
 * The switch happens on the outgoing task's stack, but current_process
 * has already been moved to the incoming task. An interrupt landing in
 * that window would let the reaper free the stack we are standing on.
 * The reaper asks about this flag before touching anything. */
static volatile int switching = 0;

static void idle_worker(void) {
    while (1) {
        asm volatile("hlt");
    }
}

void scheduler_init(void) {
    process_init();

    /* The idle task always exists, so there is always something to run.
     * uid 0: it is part of the kernel, not a user. */
    process_create("idle", idle_worker, 0);

    scheduler_enabled = 0;
}

void scheduler_start(void) {
    scheduler_enabled = 1;
}

void scheduler_set_current_process(uint32_t pid) {
    process_t *ptable = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (ptable[i].pid == pid && ptable[i].state != PROCESS_STATE_FREE) {
            current_process = &ptable[i];
            current_process_index = i;
            return;
        }
    }
}

process_t *scheduler_get_current_process(void) {
    return current_process;
}

int scheduler_is_switching(void) {
    return switching;
}

static int get_next_ready_process(int start_idx) {
    process_t *ptable = process_get_table();
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        int idx = (start_idx + i) % MAX_PROCESSES;
        if (ptable[idx].state == PROCESS_STATE_READY ||
            ptable[idx].state == PROCESS_STATE_RUNNING)
            return idx;
    }
    return -1;
}

void scheduler_tick(void) {
    if (!scheduler_enabled) return;

    process_t *ptable = process_get_table();

    /* Wake anything whose sleep has finished. */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (ptable[i].state != PROCESS_STATE_SLEEPING) continue;

        if (ptable[i].sleep_ticks > 0) ptable[i].sleep_ticks--;
        if (ptable[i].sleep_ticks == 0) ptable[i].state = PROCESS_STATE_READY;
    }

    /* DEFENCE reaper: give back the slots and stacks of dead processes.
     * Doing it here is what makes it safe - we are running on the timer
     * interrupt stack, never on the stack being freed. */
    def_reap_zombies();

    scheduler_yield();
}

void scheduler_yield(void) {
    if (!scheduler_enabled) return;

    process_t *ptable = process_get_table();

    /* DEFENCE stack_guard: if the current stack pointer has left its own
     * stack, the process is already corrupt. Retire it rather than
     * switching into a broken stack. */
    if (current_process && !def_stack_guard_check(current_process)) {
        sec_log("stack_guard", "stack pointer left its own stack");
        current_process->state = PROCESS_STATE_ZOMBIE;
    }

    int next_idx = get_next_ready_process(current_process_index != -1 ? current_process_index : 0);
    if (next_idx == -1) return;
    if (next_idx == current_process_index) return;

    process_t *prev_process = current_process;
    process_t *next_process = &ptable[next_idx];

    if (prev_process && prev_process->state == PROCESS_STATE_RUNNING)
        prev_process->state = PROCESS_STATE_READY;

    next_process->state = PROCESS_STATE_RUNNING;
    current_process = next_process;
    current_process_index = next_idx;

    /* Tell the CPU where the kernel stack for this task lives, so an
     * interrupt taken from user mode finds the right stack. A process
     * with no managed stack (kernel_main) leaves the TSS alone. */
    if (next_process->stack_base)
        set_kernel_stack(next_process->stack_base + PROCESS_STACK_SIZE);

    /* Guard the switch itself: from here until the incoming task
     * resumes, freeing any stack would pull the floor out from under
     * the call to switch_task(). */
    switching = 1;
    if (prev_process) {
        switch_task(&prev_process->esp, next_process->esp);
    } else {
        uint32_t dummy_esp;
        switch_task(&dummy_esp, next_process->esp);
    }
    switching = 0;
}
