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

#include "modules/process_management/core/process/process.h"                                    // this file
#include "modules/process_management/core/signal/signal.h"                                      // SIGKILL, signal_send_info
#include "modules/process_management/core/scheduler/scheduler.h"                                // scheduler_get_current_process, scheduler_prepare_stack, scheduler_yield
#include "modules/memory_management/core/kheap/kheap.h"                                         // kmalloc
#include "lib/string/string.h"                                                                  // memset, strncpy
#include "modules/cpu_management/core/task/cpu_task.h"                                          // cpu_task_fork
#include "modules/cpu_management/core/rlimit/rlimit.h"                                          // rlimit_nproc_ok

#include "modules/process_management/security/pid_wrap/defence/pid_bounds/def_pid_bounds.h"     // def_pid_bounds_check
#include "modules/process_management/security/fork_bomb/defence/slot_quota/def_slot_quota.h"    // def_slot_quota_check
#include "modules/process_management/security/uninit_pcb/defence/pcb_wipe/def_pcb_wipe.h"       // def_pcb_wipe
#include "modules/process_management/security/kill_system/defence/kill_guard/def_kill_guard.h"  // def_kill_allowed

static process_t table[MAX_PROCESSES];
static uint32_t  next_pid = 1;
static uint32_t  generation[MAX_PROCESSES];  // outside the PCB: pcb_wipe must not reset it

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
    p->generation = ++generation[p - table];    // a new life for this slot
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

    // Signals, as after Linux's fork + exec: the blocked mask is inherited,
    // an IGNORED signal stays ignored (that is how nohup works), a
    // handler does not survive (its code is not in the new program):
    // SIG_DFL, which pcb_wipe already left.
    p->sig_blocked = creator ? creator->sig_blocked : 0;
    for (int sig = 1; creator && sig < SIGNAL_COUNT; sig++)
        if (creator->sig_handler[sig] == SIG_IGN)
            p->sig_handler[sig] = SIG_IGN;
    p->dumpable = 1;
    p->umask = creator ? creator->umask : 002;              // others: no write
    p->no_new_privs = creator ? creator->no_new_privs : 0;   // sticky, as in Linux
    p->strace = creator ? creator->strace : 0;               // strace follows children
    if (creator && creator->seccomp_mode) {                  // a sandbox is inherited
        p->seccomp_mode   = creator->seccomp_mode;
        p->seccomp_action = creator->seccomp_action;
        for (int i = 0; i < 4; i++)
            p->seccomp_allow[i] = creator->seccomp_allow[i];
    }

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
        process_exit_with(SIGKILL);     // killing ourselves: same as exiting

    // We never free a stack here. The target becomes a zombie and the
    // reaper frees it, so there is exactly one place that frees stacks.
    target->exit_status = SIGKILL;
    target->state = PROCESS_STATE_ZOMBIE;
    process_ended(target);
    return 1;
}

void process_exit(void) {
    process_exit_with(scheduler_get_current_process()->exit_status);
}

void process_exit_with(int status) {
    // We are standing on our own stack, so we cannot free it. Become a
    // zombie and switch away for good; the reaper frees the slot later.
    process_t *self = scheduler_get_current_process();
    self->exit_status = status;
    self->state = PROCESS_STATE_ZOMBIE;
    process_ended(self);

    while (1)
        scheduler_yield();
}

// The nearest living ancestor of 'p' that asked to collect orphans
// (prctl PR_SET_CHILD_SUBREAPER), or PID 1 (idle, UzOS's "init").
static uint32_t new_parent_for(const process_t *dead) {
    const process_t *a = process_get_by_pid(dead->ppid);
    for (int depth = 0; a && depth < MAX_PROCESSES; depth++) {
        if (a->child_subreaper && a->state != PROCESS_STATE_ZOMBIE)
            return a->pid;
        a = a->ppid ? process_get_by_pid(a->ppid) : 0;
    }
    return 1;
}

void process_ended(process_t *p) {
    uint32_t heir = new_parent_for(p);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *c = &table[i];
        if (c->state == PROCESS_STATE_FREE || c == p || c->ppid != p->pid)
            continue;
        c->ppid = heir;                         // nobody is left fatherless
        if (c->pdeathsig && c->state != PROCESS_STATE_ZOMBIE)
            signal_send_info(c, c->pdeathsig, SI_KERNEL, 0);
    }
    for (int i = 0; i < MAX_PROCESSES; i++) {   // its tracees are let go
        process_t *c = &table[i];
        if (c->state != PROCESS_STATE_FREE && c->tracer == p->pid) {
            c->tracer = 0;
            signal_send_info(c, SIGCONT, SI_KERNEL, 0);
        }
    }
    process_t *parent = process_get_by_pid(p->ppid);
    if (parent)
        signal_send_info(parent, SIGCHLD, SI_KERNEL, 0);  // ignored unless it has a handler
}

int process_hangup(uint32_t sid, const process_t *keep) {
    int ended = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p == keep || p->is_system || p->sid != sid ||
            p->state == PROCESS_STATE_FREE || p->state == PROCESS_STATE_ZOMBIE)
            continue;
        uint32_t handler = p->sig_handler[SIGHUP];
        if (handler == SIG_IGN)
            continue;                           // nohup: it lives on
        if (handler != SIG_DFL) {
            signal_send(p, SIGHUP);             // its own handler decides
            continue;
        }
        p->exit_status = SIGHUP;                // the default action: end
        p->state = PROCESS_STATE_ZOMBIE;
        process_ended(p);
        ended++;
    }
    return ended;
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
