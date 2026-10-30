/*
 * atk_wait_steal.c - ATTACK wait_steal: collect someone else's child.
 *
 * Breaks:     privilege - an exit status belongs to the parent
 * Stopped by: child_only (defence/child_only)
 *
 * An exit status says what a program did (did the backup succeed, was
 * the password right?), and it is collected ONCE. A stranger who could
 * wait4() for your child would read it, and you would never see it.
 *
 * simulate(): explain.
 * launch():   the shell (root) starts "backup", which ends with exit
 *             code 3. User 1's process "thief" then asks to wait for it,
 *             as wait4(pid) does: child_only must refuse, and the record
 *             must still be there for the shell, the real parent.
 *             findings = 1 if the thief got it.
 * cleanup():  kill both, take the record.
 *
 * Uses: process_create / process_kill / process_get_by_pid (core/process),
 *       exitlog_find / exitlog_take (core/exitlog), def_wait_allowed (its
 *       defence), sys_sleep, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"      // process_create, process_kill
#include "modules/process_management/core/scheduler/scheduler.h"  // scheduler_get_current_process
#include "modules/process_management/core/exitlog/exitlog.h"      // exitlog_find, exitlog_take
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "modules/process_management/security/wait_steal/defence/child_only/def_child_only.h"  // def_wait_allowed
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define ATTACKER_UID 1

static int backup_pid = -1, thief_pid = -1;

static void backup(void) {
    process_exit_with(3 << 8);          // exit(3)
}

static void thief(void) {
    while (1)
        sys_sleep(1000);
}

static int simulate(void) {
    ui_printf("  root's \"backup\" ends with exit code 3\n");
    ui_printf("  user %d calls wait4(backup's pid) first: it steals the status\n", ATTACKER_UID);
    return 1;
}

static int launch(void) {
    backup_pid = process_create("backup", backup, 0);
    thief_pid  = process_create("thief", thief, ATTACKER_UID);
    process_t *t = thief_pid > 0 ? process_get_by_pid(thief_pid) : 0;
    if (backup_pid < 0 || !t) {
        ui_printf("  could not start the processes\n");
        return 0;
    }
    for (int i = 0; i < 50 && !exitlog_find(backup_pid, 0); i++)
        sys_sleep(10);                  // it ends; the reaper writes the record

    exit_record_t r;
    if (!exitlog_find(backup_pid, &r)) {
        ui_printf("  backup did not end\n");
        return 0;
    }
    int stolen = def_wait_allowed(t, backup_pid, r.ppid) &&
                 exitlog_take(r.ppid, backup_pid, 0, 0);
    ui_printf("  thief (user %d) -> wait4(%d)    ", ATTACKER_UID, backup_pid);
    ui_color(stolen ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", stolen ? "got exit code 3!" : "refused: not its child");
    ui_reset();
    ui_printf("  record still waiting for the parent (PID %u): %s\n", r.ppid,
              exitlog_find(backup_pid, 0) ? "yes" : "no");
    return stolen;
}

static int cleanup(void) {
    process_t *self = scheduler_get_current_process();
    int done = 0;
    if (thief_pid > 0 && process_get_by_pid(thief_pid))
        done += process_kill(thief_pid);
    if (backup_pid > 0)
        done += exitlog_take(self->pid, backup_pid, 0, 0);   // the parent collects it
    thief_pid = backup_pid = -1;
    return done;
}

static const attack_t attack = {
    "wait_steal",
    "wait4() for another process's child and take its exit status",
    INV_PRIVILEGE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "child_only"
};

void atk_wait_steal_register(void) { registry_add_attack(&attack); }
