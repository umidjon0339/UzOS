/*
 * atk_pgid_hijack.c - ATTACK pgid_hijack: move into someone else's
 * process group, or pull a stranger into yours.
 *
 * Breaks:     isolation - a login's signals reach only its own processes
 * Stopped by: session_check (defence/session_check)
 *
 * A process group gets its terminal's signals (Ctrl+C, Ctrl+Z, SIGHUP
 * at logout), and kill(-pgid) signals the whole group at once. If
 * setpgid() let any process into any group:
 *   join root's shell group  -> its Ctrl+C / SIGHUP reach you, or you
 *                               hide among root's jobs
 *   pull root's job into yours -> kill(-your_pgid) ends it
 *
 * simulate(): list the moves.
 * launch():   user 1's "spy" (its own session) asks setpgid for both
 *             moves, the way sys_setpgid does. findings = moves allowed.
 * cleanup():  kill spy and the root job.
 *
 * Uses: process_create / process_kill / process_get_by_pid (core/process),
 *       scheduler_get_current_process, def_setpgid_ok (its defence),
 *       sys_sleep, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"      // process_create, process_kill
#include "modules/process_management/core/scheduler/scheduler.h"  // scheduler_get_current_process
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "modules/process_management/security/pgid_hijack/defence/session_check/def_session_check.h"  // def_setpgid_ok
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

static int spy_pid = -1, job_pid = -1;

static void sleeper(void) {
    while (1)
        sys_sleep(1000);
}

static int simulate(void) {
    ui_printf("  setpgid(spy, root's shell group)   join another login\n");
    ui_printf("  setpgid(root's job, spy's group)   then kill(-group) ends it\n");
    return 2;
}

static int launch(void) {
    process_t *shell = scheduler_get_current_process();
    spy_pid = process_create("spy", sleeper, 1);
    job_pid = process_create("root_job", sleeper, 0);
    process_t *spy = spy_pid > 0 ? process_get_by_pid(spy_pid) : 0;
    process_t *job = job_pid > 0 ? process_get_by_pid(job_pid) : 0;
    if (!spy || !job) {
        ui_printf("  could not start the processes\n");
        return 0;
    }
    spy->sid = spy->pgid = spy->pid;        // user 1's own login session

    int join = def_setpgid_ok(spy, spy, shell->pgid);
    int pull = def_setpgid_ok(spy, job, spy->pgid);
    ui_printf("  spy joins root's group %u          ", shell->pgid);
    ui_color(join ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", join ? "allowed!" : "refused");
    ui_reset();
    ui_printf("  spy pulls root_job into group %u   ", spy->pgid);
    ui_color(pull ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", pull ? "allowed!" : "refused");
    ui_reset();
    return join + pull;
}

static int cleanup(void) {
    int killed = 0;
    if (spy_pid > 0 && process_get_by_pid(spy_pid)) killed += process_kill(spy_pid);
    if (job_pid > 0 && process_get_by_pid(job_pid)) killed += process_kill(job_pid);
    spy_pid = job_pid = -1;
    return killed;
}

static const attack_t attack = {
    "pgid_hijack",
    "setpgid into another session's group, or pull a stranger into yours",
    INV_ISOLATION, SEV_MEDIUM,
    simulate, launch, cleanup,
    "session_check"
};

void atk_pgid_hijack_register(void) { registry_add_attack(&attack); }
