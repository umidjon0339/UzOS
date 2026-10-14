#include "proc_security.h"
#include "process.h"
#include "scheduler.h"
#include "vga.h"

/* ===================================================================
 * ATTACK: kill init bypass
 * ===================================================================
 * Break INV_PRIVILEGE: a normal caller must not be able to destroy a
 * protected system process.
 *
 * The PID-1 protection lived inside the shell command cmd_kill():
 *
 *     if (pid == 1) { print("Cannot kill ..."); return; }
 *
 * That guards one door only. process_kill() is a public kernel
 * function, so a syscall, a driver or another module can call it
 * directly - no shell, no check. Here we do exactly that.
 *
 * Lesson: a guard belongs next to the operation it protects, not next
 * to the user interface that calls it first.
 * =================================================================== */

#define SYSTEM_PID    1   /* the PID the shell refuses to kill */
#define ATTACK_BUDGET 4   /* attempts one live run is allowed  */

/* simulate() reads the table and changes nothing at all. */
static int atk_kill_init_bypass_simulate(void) {
    process_t *t    = process_get_table();
    process_t *self = scheduler_get_current_process();
    uint32_t me = self ? self->pid : 0;
    int reachable = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (t[i].state == PROCESS_STATE_FREE) continue;
        if (t[i].pid != me) reachable++;  /* our own PID is no target */
    }

    vga_print_str("  caller pid            : ");
    vga_print_int((int)me);
    vga_print_str("\n  protected pid         : ");
    vga_print_int(SYSTEM_PID);
    vga_print_str(process_get_by_pid(SYSTEM_PID) ? " (alive)\n" : " (not running)\n");
    vga_print_str("  shell cmd_kill check  : refuses pid 1\n");
    vga_print_str("  direct process_kill() : no shell check on this path\n");
    vga_print_str("  reachable by a caller : ");
    vga_print_int(reachable);
    vga_print_str("\n  (nothing was killed - simulate() is read-only)\n");
    return reachable;
}

/* launch() makes the real calls: this IS the bypass, because the shell
 * command never runs. It stays bounded (fixed attempt count) and never
 * aims at the process we are running on, which is an absolute rule.
 * With kill_guard live, every attempt is refused inside the kernel. */
static int atk_kill_init_bypass_launch(void) {
    process_t *self = scheduler_get_current_process();
    uint32_t me = self ? self->pid : 0;

    if (me == SYSTEM_PID) {  /* never target the running process */
        vga_print_str("  we ARE pid 1 - refusing to target ourselves\n");
        return 0;
    }
    if (!process_get_by_pid(SYSTEM_PID)) {
        vga_print_str("  no pid 1 on this system - nothing to demonstrate\n");
        return 0;
    }

    uint32_t before = sec_total_denied();

    /* No shell, no cmd_kill(), no pid==1 check - just the call. */
    for (int i = 0; i < ATTACK_BUDGET; i++) process_kill(SYSTEM_PID);

    uint32_t refused = sec_total_denied() - before;
    vga_print_str("  direct process_kill(1) calls : ");
    vga_print_int(ATTACK_BUDGET);
    vga_print_str("\n  refused inside the kernel    : ");
    vga_print_int((int)refused);
    vga_print_str("\n");
    return ATTACK_BUDGET;
}

/* No cleanup: the attack only calls the kernel and the guard refuses,
 * so the process table is untouched. If a kill did get through, no
 * cleanup could bring a dead process back anyway. */
static const attack_t atk = {
    "kill_init_bypass",
    "the shell's PID-1 guard is bypassed by calling process_kill() directly",
    INV_PRIVILEGE,
    SEV_CRITICAL,
    atk_kill_init_bypass_simulate,
    atk_kill_init_bypass_launch,
    0,
    "kill_guard"
};

void atk_kill_init_bypass_register(void) {
    proc_security_register_attack(&atk);
}

void atk_kill_init_register(void) {
    proc_security_register_attack(&atk);
}
