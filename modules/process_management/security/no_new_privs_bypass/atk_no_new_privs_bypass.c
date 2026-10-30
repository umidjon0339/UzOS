/*
 * atk_no_new_privs_bypass.c - ATTACK no_new_privs_bypass: use a sandbox
 * without the promise, or take the promise back.
 *
 * Breaks:     privilege - a filter must never be aimed at a stronger program
 * Stopped by: no_new_privs (defence/no_new_privs)
 *
 * A seccomp filter makes chosen syscalls fail. Installed by a normal
 * user and inherited by a MORE privileged program (on Linux: a setuid
 * one), "setuid() failed but I carry on as root" is a classic hole.
 * So a filter needs no_new_privs first - and that promise must stick:
 * no taking it back, and every child keeps it.
 *
 * simulate(): list the tricks.
 * launch():   run no_new_privs_bypass.s as user 1 (see its header).
 *             findings = tricks that worked.
 * cleanup():  kill it if still running.
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox (core/usertask),
 *       process_kill / process_get_table (core/process), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/usertask/usertask.h"    // usertask_start, usertask_wait, usertask_mailbox
#include "modules/process_management/core/process/process.h"      // process_kill, process_get_table
#include "lib/string/string.h"                                    // strcmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define ATTACKER_UID 1
#define WAIT_MS      500

// From no_new_privs_bypass.s
extern uint8_t nnp_program[], nnp_program_end[];

static int simulate(void) {
    ui_printf("  1. seccomp filter without no_new_privs\n");
    ui_printf("  2. set no_new_privs, then take it back\n");
    ui_printf("  3. a child that does not have it any more\n");
    return 3;
}

static void verdict(const char *what, int bad, const char *text) {
    ui_printf("  %-36s ", what);
    ui_color(bad ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", text);
    ui_reset();
}

static int launch(void) {
    volatile uint32_t *box = usertask_mailbox();
    int pid = usertask_start("nnp_test", nnp_program, nnp_program_end, ATTACKER_UID);
    if (pid < 0) {
        ui_printf("  could not start the program\n");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);
    if (box[0] != 1) {
        ui_printf("  the program did not finish\n");
        return 0;
    }
    int filter = (int32_t)box[1] == 0;
    int undo   = (int32_t)box[3] == 0;
    int lost   = box[4] != 1;
    verdict("seccomp filter, no promise", filter, filter ? "installed!" : "refused");
    verdict("prctl(NO_NEW_PRIVS, 0): take it back", undo, undo ? "taken back!" : "refused (sticky)");
    verdict("its child's no_new_privs", lost, lost ? "lost!" : "1 (inherited)");
    return filter + undo + lost;
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int killed = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].state != PROCESS_STATE_ZOMBIE &&
            strcmp(table[i].name, "nnp_test") == 0)
            killed += process_kill(table[i].pid);
    return killed;
}

static const attack_t attack = {
    "no_new_privs_bypass",
    "a seccomp filter without no_new_privs; take the promise back",
    INV_PRIVILEGE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "no_new_privs"
};

void atk_no_new_privs_bypass_register(void) { registry_add_attack(&attack); }
