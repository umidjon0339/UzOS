/*
 * atk_pid_reuse_race.c - ATTACK pid_reuse_race: signal the wrong process.
 *
 * Breaks:     identity - "that process" must not turn into another one
 * Stopped by: pidfd_handle (defence/pidfd_handle), with pcb_generation
 *
 * `pgrep worker` ... `kill 1234`: between the two the worker may end,
 * and where PIDs wrap around (Linux: pid_max) 1234 can come back as a
 * new process, which then gets the signal. A handle that remembered only
 * WHERE the process was (its slot) has the same problem in UzOS: slots
 * are reused.
 *
 * simulate(): explain the race.
 * launch():   as the shell (root):
 *               1. start "race_victim", pidfd_open() it
 *               2. it ends, the reaper frees the slot
 *               3. start "race_new" until it lands in the same slot
 *               4. pidfd_send_signal(fd, SIGSTOP): must fail, and
 *                  race_new must still be running
 *               5. as user 1, pidfd_getfd() out of a root process: refused
 *             findings = 1 per step that reached the wrong process.
 * cleanup():  kill race_new and race_victim, close the pidfd.
 *
 * Uses: sys_pidfd_open / sys_pidfd_send_signal / sys_close (syscalls),
 *       process_create / process_kill / process_get_by_pid (core/process),
 *       def_reap_zombies (zombie_leak/defence/reaper), def_pidfd_getfd_ok
 *       (its defence), sys_sleep, ui.
 */

#include "modules/security_management/core/registry/registry.h"                   // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"                      // process_create, process_kill
#include "modules/process_management/core/signal/signal.h"                        // SIGSTOP
#include "modules/process_management/syscalls/pidfd_open/sys_pidfd_open.h"        // sys_pidfd_open
#include "modules/process_management/syscalls/pidfd_send_signal/sys_pidfd_send_signal.h"  // sys_pidfd_send_signal
#include "modules/io_management/syscalls/close/sys_close.h"                       // sys_close
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                  // sys_sleep
#include "modules/process_management/security/zombie_leak/defence/reaper/def_reaper.h"  // def_reap_zombies
#include "modules/process_management/security/pid_reuse_race/defence/pidfd_handle/def_pidfd_handle.h"  // def_pidfd_getfd_ok
#include "lib/string/string.h"                                                    // strcmp
#include "modules/ui/core/style/ui.h"                                             // ui_printf, ui_color

#define TRIES 8

static int pidfd = -1;

static void sleeper(void) {
    while (1)
        sys_sleep(1000);
}

static int simulate(void) {
    ui_printf("  pgrep worker -> 1234 ... worker ends ... 1234 is reused\n");
    ui_printf("  kill 1234 now hits the NEW process\n");
    ui_printf("  a pidfd must keep naming the old one (and then fail)\n");
    return 1;
}

static void verdict(const char *what, int bad, const char *bad_text, const char *good_text) {
    ui_printf("  %-38s ", what);
    ui_color(bad ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", bad ? bad_text : good_text);
    ui_reset();
}

static int launch(void) {
    int victim = process_create("race_victim", sleeper, 0);
    process_t *slot = victim > 0 ? process_get_by_pid(victim) : 0;
    pidfd = slot ? sys_pidfd_open(victim, 0) : -1;
    if (pidfd < 0) {
        ui_printf("  could not start the victim / open a pidfd\n");
        return 0;
    }
    ui_printf("  race_victim PID %d, pidfd %d\n", victim, pidfd);

    process_kill(victim);
    def_reap_zombies();
    for (int i = 0; i < TRIES && slot->state == PROCESS_STATE_FREE; i++)
        process_create("race_new", sleeper, 0);
    if (slot->state == PROCESS_STATE_FREE) {
        ui_printf("  the slot was not reused (try again)\n");
        return 0;
    }
    ui_printf("  race_new PID %u took the same slot\n", slot->pid);

    int sent = sys_pidfd_send_signal(pidfd, SIGSTOP) == 0;
    int hit  = sent || slot->state == PROCESS_STATE_STOPPED;
    verdict("pidfd_send_signal(old pidfd, SIGSTOP)", hit, "race_new was stopped!",
            "failed (ESRCH): race_new untouched");

    int stolen = def_pidfd_getfd_ok(1, slot);       // user 1 -> root's process
    verdict("user 1: pidfd_getfd from root's race_new", stolen, "allowed!", "refused");
    return hit + stolen;
}

static int cleanup(void) {
    process_t *table = process_get_table();
    int done = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state != PROCESS_STATE_FREE && p->state != PROCESS_STATE_ZOMBIE &&
            (strcmp(p->name, "race_victim") == 0 || strcmp(p->name, "race_new") == 0))
            done += process_kill(p->pid);
    }
    if (pidfd >= 0 && sys_close(pidfd) == 0)
        done++;
    pidfd = -1;
    return done;
}

static const attack_t attack = {
    "pid_reuse_race",
    "a stale pid / handle signals whoever took the process's place",
    INV_IDENTITY, SEV_HIGH,
    simulate, launch, cleanup,
    "pidfd_handle"
};

void atk_pid_reuse_race_register(void) { registry_add_attack(&attack); }
