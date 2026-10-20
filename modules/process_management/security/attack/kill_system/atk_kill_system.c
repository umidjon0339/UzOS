/*
 * atk_kill_system.c - ATTACK kill_system: kill a system process directly.
 *
 * Breaks:     privilege - a caller must not destroy the kernel's own tasks
 * Stopped by: kill_guard (security/defence/kill_guard)
 *
 * The idea: a check in the `kill` shell command guards only that one door.
 * process_kill() is a public kernel function, so calling it directly
 * (as a syscall or another module would) skips the shell completely.
 *
 * simulate(): list the system processes that would be targets.
 * launch():   call process_kill() on each system process except ourselves.
 *
 * Uses: process_kill / process_get_table (core/process),
 *       scheduler_get_current_process (core/scheduler).
 */

#include "security_lab.h"
#include "process.h"
#include "scheduler.h"
#include "vga.h"

// Call 'visit' for every system process that is not the caller.
static int for_each_target(void (*visit)(process_t *)) {
    process_t *table = process_get_table();
    process_t *self  = scheduler_get_current_process();
    int n = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE || !p->is_system || p == self)
            continue;               // never aim at ourselves
        visit(p);
        n++;
    }
    return n;
}

static void show_target(process_t *p) {
    vga_print_str("  target: PID ");
    vga_print_int(p->pid);
    vga_print_str(" ");
    vga_print_str(p->name);
    vga_print_str("\n");
}

static void try_kill(process_t *p) {
    show_target(p);
    vga_print_str(process_kill(p->pid) ? "    -> KILLED, guard bypassed\n"
                                       : "    -> refused inside process_kill()\n");
}

static int simulate(void) {
    int n = for_each_target(show_target);
    vga_print_str("  process_kill() is a public function: no shell check on this path\n");
    return n;
}

static int launch(void) {
    return for_each_target(try_kill);
}

static const attack_t attack = {
    "kill_system",
    "call process_kill() directly to destroy a system process",
    INV_PRIVILEGE, SEV_CRITICAL,
    simulate, launch, 0,        // nothing to undo: a dead process cannot be revived
    "kill_guard"
};

void atk_kill_system_register(void) {
    proc_security_register_attack(&attack);
}
