/*
 * atk_output_spoof.c - ATTACK output_spoof: a name that fakes a table row.
 *
 * Breaks:     consistency - a table shows exactly what is in the kernel
 * Stopped by: safe_text (security/defence/safe_text)
 *
 * `ps` prints one row per process, name last. A name is just bytes, so
 * a process can be named "x\n  1  RUNNING  0  init": printed raw, the
 * '\n' starts a new line that looks like a real process. The user now
 * sees a process that does not exist (and can hide one that does).
 *
 * simulate(): show how the raw name would print.
 * launch():   start a real process with that name, run the real `ps`,
 *             then stop the process again.
 *
 * Uses: process_create / process_kill (process_management), cmd_ps
 *       (cpu_management/commands/ps).
 */

#include "registry.h"
#include "audit.h"
#include "process.h"
#include "modules/cpu_management/commands/ps/cmd_ps.h"     // cmd_ps
#include "sys_sleep.h"
#include "sys_exit.h"
#include "vga.h"

#define FAKE_NAME "x\n      1  RUNNING    0  init"

static void sleeper(void) {
    sys_sleep(2000);
    sys_exit();
}

static int simulate(void) {
    vga_print_str("  process name: \"x\\n      1  RUNNING    0  init\"\n");
    vga_print_str("  printed raw, ps would show:\n");
    vga_print_str("      7  READY      0  x\n");
    vga_print_str("      1  RUNNING    0  init      <- a process that does not exist\n");
    return 1;
}

static int launch(void) {
    uint32_t denied = audit_total();

    int pid = process_create(FAKE_NAME, sleeper, 0);
    if (pid < 0) {
        vga_print_str("  could not start the test process\n");
        return 0;
    }
    vga_print_str("  started a process with a newline in its name; ps shows:\n");
    cmd_ps("");
    process_kill(pid);
    return audit_total() - denied;
}

static const attack_t attack = {
    "output_spoof",
    "a process name with a newline adds a fake row to ps",
    INV_CONSISTENCY, SEV_MEDIUM,
    simulate, launch, 0,        // launch stops its own process
    "safe_text"
};

void atk_output_spoof_register(void) {
    registry_add_attack(&attack);
}
