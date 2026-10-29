/*
 * atk_signal_abuse.c - ATTACK signal_abuse: signal processes you may not.
 *
 * Breaks:     privilege - a signal must reach only processes you own
 * Stopped by: signal_permission (defence/signal_permission)
 *
 * A signal (SIGINT, SIGKILL, ...) stops or disturbs a process. If any
 * user could signal any process, one user could interrupt another's
 * work, or poke the kernel's own processes. The rule lets a signal reach
 * only your own processes, unless you hold CAP_KILL, and never a system
 * process.
 *
 * simulate(): name the signals a stranger must not be able to send.
 * launch():   as user 1, ask to signal user 2's process and a system
 *             process; both are refused. 0 findings = the rule holds.
 * cleanup():  nothing (launch only reads the rule).
 *
 * Uses: signal_permission (its defence), vga.
 */

#include "registry.h"
#include "audit.h"
#include "def_signal_permission.h"
#include "vga.h"

static int simulate(void) {
    vga_print_str("  a signal (SIGINT/SIGKILL) stops or disturbs a process\n");
    vga_print_str("  user 1 must not signal user 2, or a system process\n");
    vga_print_str("  the rule: your own processes only (CAP_KILL for more)\n");
    return 1;
}

static int launch(void) {
    uint32_t before = audit_total();
    def_signal_allowed(1, 2, 0);    // user 1 -> user 2's process: refused
    def_signal_allowed(1, 0, 1);    // user 1 -> a system process: refused
    uint32_t refused = audit_total() - before;
    vga_print_str("  cross-user / system signals refused: ");
    vga_print_int((int)refused); vga_print_str(" of 2\n");
    return (int)(2 - refused);
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "signal_abuse",
    "send a signal to another user's or the kernel's processes",
    INV_PRIVILEGE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "signal_permission"
};

void atk_signal_abuse_register(void) { registry_add_attack(&attack); }
