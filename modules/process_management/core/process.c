#include "process.h"
#include "scheduler.h"
#include "kheap.h"
#include "string.h"
#include "vga.h"
#include "proc_security.h"

/* ===================================================================
 * Process table.
 *
 * Every defence in the Process Security Lab is called from this file,
 * at the exact point where its attack would strike. Grep "DEFENCE"
 * to find all of them.
 * =================================================================== */

static process_t process_table[MAX_PROCESSES];
static uint32_t  next_pid = 1;

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        def_pcb_wipe(&process_table[i]);
        process_table[i].state = PROCESS_STATE_FREE;
    }
}

static int find_free_process_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (process_table[i].state == PROCESS_STATE_FREE) return i;
    return -1;
}

/* Called when a worker function returns.
 *
 * We must NOT free our own stack here: we are still standing on it.
 * Mark the slot a zombie and let the reaper (which runs later, from
 * the timer interrupt and therefore on a different stack) reclaim it. */
void process_exit(void) {
    process_t *self = scheduler_get_current_process();
    if (self) self->state = PROCESS_STATE_ZOMBIE;

    while (1) {
        scheduler_yield();
    }
}

int process_create(const char *name, void (*entry_point)(void), uint32_t uid) {
    /* DEFENCE pid_bounds: the PID counter must never wrap around,
     * otherwise a new process could be handed the PID of a live one. */
    if (!def_pid_bounds_check(next_pid)) {
        vga_print_str("[SECURITY] PID space exhausted, refusing to create.\n");
        return -1;
    }

    /* DEFENCE slot_quota: bound how much of the table one user can take. */
    if (!def_slot_quota_check(uid)) return -1;

    int slot = find_free_process_slot();
    if (slot == -1) return -1;

    process_t *p = &process_table[slot];

    /* DEFENCE pcb_wipe: a recycled slot must never leak the fields of
     * its previous owner (cwd, fd_table, ...) into the new process. */
    def_pcb_wipe(p);

    p->pid   = next_pid++;
    p->uid   = uid;
    p->state = PROCESS_STATE_READY;

    if (!name) name = "?";
    strncpy(p->name, name, 31);
    p->name[31] = '\0';

    /* A process created with no entry point is the thread that is
     * already running (kernel_main). It executes on the boot stack, not
     * on a stack we own, so it gets no managed stack at all. The stack
     * guard treats stack_base == 0 as "unguarded" and skips it. */
    if (!entry_point) {
        p->stack_base = 0;
        p->esp        = 0;
        return p->pid;
    }

    p->stack_base = (uint32_t)kmalloc(PROCESS_STACK_SIZE);
    if (!p->stack_base) {
        p->state = PROCESS_STATE_FREE;
        return -1;
    }

    /* Do not hand a new process someone else's old data. */
    memset((void *)p->stack_base, 0, PROCESS_STACK_SIZE);

    /* Build the initial stack that switch_task will "return" into. */
    uint32_t *stack = (uint32_t *)(p->stack_base + PROCESS_STACK_SIZE);

    *(--stack) = (uint32_t)process_exit;   /* return target         */
    *(--stack) = (uint32_t)entry_point;    /* popped by the wrapper */

    extern void task_start_wrapper(void);
    *(--stack) = (uint32_t)task_start_wrapper;

    *(--stack) = 0;  /* ebp */
    *(--stack) = 0;  /* ebx */
    *(--stack) = 0;  /* esi */
    *(--stack) = 0;  /* edi */

    p->esp = (uint32_t)stack;
    return p->pid;
}

int process_fork(void) {
    process_t *parent = scheduler_get_current_process();
    if (!parent) return -1;

    /* Fork bomb limit: one process may not have unlimited children. */
    if (parent->child_count >= MAX_CHILDREN_PER_PROCESS && parent->uid != 0) {
        vga_print_str("\n[SECURITY] Fork Bomb prevented! Process reached child limit.\n");
        return -1;
    }

    int slot = find_free_process_slot();
    /* Keep a few slots for root so the machine stays recoverable. */
    if (slot >= MAX_PROCESSES - 5 && parent->uid != 0) {
        vga_print_str("\n[SECURITY] Fork prevented! Reserved for Root.\n");
        return -1;
    }
    if (slot == -1) return -1;

    process_t *child = &process_table[slot];
    def_pcb_wipe(child);

    child->pid        = next_pid++;
    child->parent_pid = parent->pid;
    child->uid        = parent->uid;
    child->state      = PROCESS_STATE_READY;
    strncpy(child->name, parent->name, 31);

    child->stack_base = (uint32_t)kmalloc(PROCESS_STACK_SIZE);
    if (!child->stack_base) {
        /* DEFENCE fork_cleanup: every failure path must give the slot
         * back, otherwise a repeated fork attempt leaks the heap. */
        def_release_child_slot(child);
        return -1;
    }

    /* A real fork also has to copy the page directory and the register
     * set. Until that exists we must not hand out a half-built process,
     * so undo everything and report failure. */
    def_release_child_slot(child);
    vga_print_str("Fork limits checked, but true fork needs a ring 3 clone.\n");
    return -1;
}

int process_kill(uint32_t pid) {
    process_t *caller   = scheduler_get_current_process();
    uint32_t caller_pid = caller ? caller->pid : 0;

    /* DEFENCE kill_guard / self_kill: decide whether this kill is
     * allowed at all. This lives here, not in the shell command, so it
     * cannot be bypassed by calling process_kill() directly. */
    if (!def_kill_allowed(caller_pid, pid)) return 0;
    if (!def_pid_bounds_check(pid))         return 0;

    process_t *p = process_get_by_pid(pid);
    if (!p) return 0;

    /* DEFENCE defer_self_kill: freeing the stack we are standing on
     * would corrupt the very code doing the freeing. Mark the slot and
     * step aside instead, so the reaper can reclaim it safely. */
    if (!def_self_kill_policy(caller_pid, pid)) {
        p->state = PROCESS_STATE_ZOMBIE;
        scheduler_yield();
        return 1;
    }

    /* Someone else's stack: still defer, so there is exactly one place
     * that ever frees a stack (the reaper) and it always runs on a
     * different stack than the one it is freeing. */
    p->state = PROCESS_STATE_ZOMBIE;
    return 1;
}

process_t *process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_STATE_FREE)
            return &process_table[i];
    return 0;
}

process_t *process_get_table(void) {
    return process_table;
}
