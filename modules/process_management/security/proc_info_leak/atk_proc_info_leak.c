/*
 * atk_proc_info_leak.c - ATTACK proc_info_leak: read another user's process.
 *
 * Breaks:     disclosure - what a user is doing is their business
 * Stopped by: hidepid (defence/hidepid)
 *
 * The kernel knows each process's working directory, open files and
 * signal masks. A tool like procinfo, pwdx or /proc/<pid>/status that
 * shows them to anyone tells a stranger which files you edit and where
 * your secrets are.
 *
 * simulate(): list what a stranger would learn.
 * launch():   start a root process "secret_job", then ask hidepid as
 *             user 1 exactly as procinfo, pwdx and /proc do. Also ask
 *             about user 1's own process (that one must be visible).
 *             findings = root details shown to user 1.
 * cleanup():  kill secret_job.
 *
 * Uses: process_create / process_kill / process_get_by_pid
 *       (core/process), def_hidepid_visible (its defence), sys_sleep, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"      // process_create, process_kill, process_get_by_pid
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "modules/process_management/security/proc_info_leak/defence/hidepid/def_hidepid.h"  // def_hidepid_visible
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define ATTACKER_UID 1
#define ROOT_UID     0

static int secret_pid = -1, own_pid = -1;

static void sleeper(void) {
    while (1)
        sys_sleep(1000);
}

static int simulate(void) {
    ui_printf("  procinfo / pwdx / /proc/<pid>/status of root's process:\n");
    ui_printf("  its directory, open files, signal masks, kernel stack\n");
    ui_printf("  user %d would learn what root is working on\n", ATTACKER_UID);
    return 1;
}

static int launch(void) {
    secret_pid = process_create("secret_job", sleeper, ROOT_UID);
    own_pid    = process_create("my_job", sleeper, ATTACKER_UID);
    process_t *secret = secret_pid > 0 ? process_get_by_pid(secret_pid) : 0;
    process_t *own    = own_pid > 0 ? process_get_by_pid(own_pid) : 0;
    if (!secret || !own) {
        ui_printf("  could not start the processes\n");
        return 0;
    }

    int leaked = def_hidepid_visible(ATTACKER_UID, secret);
    int mine   = def_hidepid_visible(ATTACKER_UID, own);
    ui_printf("  user %d -> root's secret_job (PID %d)   ", ATTACKER_UID, secret_pid);
    ui_color(leaked ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", leaked ? "details shown!" : "hidden");
    ui_reset();
    ui_printf("  user %d -> its own my_job (PID %d)      ", ATTACKER_UID, own_pid);
    ui_color(mine ? UI_GOOD : UI_BAD);
    ui_printf("%s\n", mine ? "shown (its own)" : "hidden (too strict!)");
    ui_reset();
    return leaked;
}

static int cleanup(void) {
    int killed = 0;
    if (secret_pid > 0 && process_get_by_pid(secret_pid))
        killed += process_kill(secret_pid);
    if (own_pid > 0 && process_get_by_pid(own_pid))
        killed += process_kill(own_pid);
    secret_pid = own_pid = -1;
    return killed;
}

static const attack_t attack = {
    "proc_info_leak",
    "a user reads root's process details: directory, files, signals",
    INV_DISCLOSURE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "hidepid"
};

void atk_proc_info_leak_register(void) { registry_add_attack(&attack); }
