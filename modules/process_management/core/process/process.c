/*
 * process.c - Create, kill and end processes.
 *
 * Uses: the scheduler (who is running, build a first stack), kmalloc for
 *       stacks (memory_management), cpu_management for what a new
 *       process inherits (core/task: nice, policy, affinity, limits) and
 *       how many processes a user may have (core/rlimit: RLIMIT_NPROC),
 *       and four defences that guard the exact moments an attack would strike:
 *         pid_bounds  - a PID is never reused         (create, kill)
 *         slot_quota  - one user cannot fill the table (create)
 *         pcb_wipe    - a reused slot starts empty     (init, create)
 *         kill_guard  - you may only kill what you own (kill)
 */

#include "process.h"
#include "scheduler.h"
#include "kheap.h"
#include "string.h"
#include "modules/cpu_management/core/task/cpu_task.h"      // cpu_task_fork
#include "modules/cpu_management/core/rlimit/rlimit.h"      // rlimit_nproc_ok

#include "def_pid_bounds.h"
#include "def_slot_quota.h"
#include "def_pcb_wipe.h"
#include "def_kill_guard.h"

static process_t table[MAX_PROCESSES];
static uint32_t  next_pid = 1;

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        def_pcb_wipe(&table[i]);
}

static process_t *find_free_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state == PROCESS_STATE_FREE)
            return &table[i];
    return 0;
}

// The first code every new process runs: switch_task() "returns" here.
static void process_start(void) {
    process_t *self = scheduler_get_current_process();
    self->entry();
    process_exit();         // entry returned: the process is finished
}

int process_create(const char *name, void (*entry)(void), uint32_t uid) {
    // DEFENCE pid_bounds: refuse before the PID counter can wrap around.
    if (!def_pid_bounds_check(next_pid))
        return -1;

    // DEFENCE slot_quota: one user may not take every slot.
    if (!def_slot_quota_check(uid))
        return -1;

    // RLIMIT_NPROC (cpu_management): the creator's own limit for this user.
    process_t *creator = scheduler_get_current_process();
    if (!rlimit_nproc_ok(creator, uid))
        return -1;

    process_t *p = find_free_slot();
    if (!p)
        return -1;

    // DEFENCE pcb_wipe: nothing from the slot's last owner survives.
    def_pcb_wipe(p);

    if (entry) {
        p->stack_base = (uint32_t)kmalloc(PROCESS_STACK_SIZE);
        if (!p->stack_base)
            return -1;      // slot is still FREE

        // Old heap data must not leak into the new stack either.
        memset((void *)p->stack_base, 0, PROCESS_STACK_SIZE);
        p->esp = scheduler_prepare_stack(p->stack_base + PROCESS_STACK_SIZE,
                                         process_start);
    }

    p->pid   = next_pid++;
    p->uid   = uid;
    p->entry = entry;
    strncpy(p->name, name ? name : "?", PROCESS_NAME_LEN - 1);

    // nice, policy, CPU mask, limits, ppid, ...: from the creator
    // (cpu_management/core/task), as fork() copies them in Unix.
    cpu_task_fork(p, creator);

    // process group and session: inherit from the creator, or (no
    // creator, e.g. a session leader) default to the process's own pid.
    p->pgid = creator ? creator->pgid : p->pid;
    p->sid  = creator ? creator->sid  : p->pid;

    p->state = PROCESS_STATE_READY;     // last: now the scheduler may run it
    return p->pid;
}

int process_kill(uint32_t pid) {
    // DEFENCE pid_bounds: 0 and out-of-range PIDs name no process.
    if (!def_pid_bounds_check(pid))
        return 0;

    process_t *caller = scheduler_get_current_process();
    process_t *target = process_get_by_pid(pid);

    // DEFENCE kill_guard: checked here, not in the shell, so every
    // caller (shell, syscall, another module) goes through it.
    if (!def_kill_allowed(caller, target))
        return 0;

    if (target == caller)
        process_exit();     // killing ourselves: same as exiting

    // We never free a stack here. The target becomes a zombie and the
    // reaper frees it, so there is exactly one place that frees stacks.
    target->state = PROCESS_STATE_ZOMBIE;
    return 1;
}

void process_exit(void) {
    // We are standing on our own stack, so we cannot free it. Become a
    // zombie and switch away for good; the reaper frees the slot later.
    scheduler_get_current_process()->state = PROCESS_STATE_ZOMBIE;

    while (1)
        scheduler_yield();
}

uint32_t process_caller_uid(void) {
    process_t *self = scheduler_get_current_process();
    return self ? self->uid : 0;
}

process_t *process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].pid == pid)
            return &table[i];
    return 0;
}

process_t *process_get_table(void) {
    return table;
}

int process_count_free_slots(void) {
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state == PROCESS_STATE_FREE)
            n++;
    return n;
}
