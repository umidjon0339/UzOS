/*
 * atk_ptrace_abuse.c - ATTACK ptrace_abuse: debug a process you may not.
 *
 * Breaks:     disclosure - a debugger sees all of a process's memory
 * Stopped by: ptrace_restrict (defence/ptrace_restrict)
 *
 * A debugging interface like ptrace() reads and writes another process's
 * registers and memory. If any user could attach to any process, ptrace
 * would be a window into everyone's secrets - keys, passwords, tokens.
 * UzOS has no ptrace() yet; this shows the gate one would have to pass:
 * ownership (or a debug capability), and never a system process.
 *
 * simulate(): explain what a debugger can see, and why it needs rights.
 * launch():   as user 1, ask to attach to user 2's process and a system
 *             process; both refused. 0 findings = the rule holds.
 * cleanup():  nothing.
 *
 * Uses: ptrace_restrict (its defence), vga.
 */

#include "registry.h"
#include "audit.h"
#include "def_ptrace_restrict.h"
#include "vga.h"

static int simulate(void) {
    vga_print_str("  a debugger reads/writes a process's registers + memory\n");
    vga_print_str("  so attaching to another user's process leaks its secrets\n");
    vga_print_str("  the rule: your own processes only (and never a system one)\n");
    return 1;
}

static int launch(void) {
    uint32_t before = audit_total();
    def_ptrace_allowed(1, 2, 0);    // attach to user 2: refused
    def_ptrace_allowed(1, 0, 1);    // attach to a system process: refused
    uint32_t refused = audit_total() - before;
    vga_print_str("  foreign ptrace attaches refused: ");
    vga_print_int((int)refused); vga_print_str(" of 2\n");
    return (int)(2 - refused);
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "ptrace_abuse",
    "attach a debugger to a process you do not own to read its memory",
    INV_DISCLOSURE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "ptrace_restrict"
};

void atk_ptrace_abuse_register(void) { registry_add_attack(&attack); }
