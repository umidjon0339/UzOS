/*
 * atk_fork_bomb.c - ATTACK fork_bomb: fork until the table is full.
 *
 * Breaks:     resource - process slots are finite and shared
 * Stopped by: process_quota (defence/process_quota)
 *
 * `while (1) fork();` doubles the number of processes about every
 * instant. Without a limit it fills the whole process table, and then
 * nobody - not even root - can start a program to kill the bomb. This
 * is the CPU module's view of the classic bomb: the fix here is the
 * per-USER limit (RLIMIT_NPROC / `ulimit -u`), separate from the
 * total-table guard in process_management (slot_quota). Its id is
 * `cpu_fork_bomb`, because `fork_bomb` is already taken there.
 *
 * simulate(): show the doubling and where the per-user cap stops it.
 * launch():   ask the real rule as a user at the cap (refused) and as
 *             root (allowed). 0 findings = the cap holds.
 * cleanup():  nothing (launch only reads the rule; it forks nothing).
 *
 * Uses: process_quota (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/fork_bomb/defence/process_quota/def_process_quota.h"
#include "modules/ui/core/style/ui.h"

#define CAP 4

static int simulate(void) {
    ui_printf("  fork() doubles processes: 1, 2, 4, 8, ... until full\n");
    ui_printf("  with `ulimit -u %d` the %dth process for a user is refused\n",
              CAP, CAP + 1);
    ui_printf("  root keeps a reserve, so an admin can still clean up\n");
    return 1;
}

static int launch(void) {
    int findings = 0;
    if (def_nproc_quota_ok(0, CAP, CAP)) findings++;    // user at the cap: must refuse
    if (!def_nproc_quota_ok(1, 999, CAP)) findings++;   // root: must allow
    ui_printf("  user at the cap asks to fork again: %s\n",
              def_nproc_quota_ok(0, CAP, CAP) ? "ALLOWED" : "refused");
    return findings;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "cpu_fork_bomb",
    "fork() in a loop to fill the process table for everyone",
    INV_RESOURCE, SEV_HIGH,
    simulate, launch, cleanup,
    "process_quota"
};

void atk_cpu_fork_bomb_register(void) { registry_add_attack(&attack); }
