/*
 * atk_core_dump_leak.c - ATTACK core_dump_leak: read what a process held
 * before it dropped its rights.
 *
 * Breaks:     disclosure - secrets read as root stay root's
 * Stopped by: dumpable_flag (defence/dumpable_flag)
 *
 * A daemon starts as root, reads its private key into memory, then
 * drops to user 1 with setuid(). The key is still in its memory, and the
 * process now belongs to user 1 - who can read it with a debugger,
 * process_vm_readv, or (on Linux) the core file of a crash.
 *
 * simulate(): the story.
 * launch():   start "daemon" (root), put a "key" in memory, let it
 *             setuid(1) the way sys_setuid does (dumpable_flag runs),
 *             then user 1's "thief" asks to read the key with
 *             process_vm_readv's check. findings = 1 if allowed.
 * cleanup():  kill both.
 *
 * Uses: process_create / process_kill / process_get_by_pid (core/process),
 *       def_dumpable_on_setuid (its defence), def_vm_access_ok
 *       (security/memory_peek/defence/vm_access_check), USERTASK_STACK
 *       (core/usertask: a user page for the "key"), sys_sleep, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/core/process/process.h"      // process_create, process_kill
#include "modules/process_management/core/usertask/usertask.h"    // USERTASK_STACK
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "modules/process_management/security/core_dump_leak/defence/dumpable_flag/def_dumpable_flag.h"  // def_dumpable_on_setuid
#include "modules/process_management/security/memory_peek/defence/vm_access_check/def_vm_access_check.h"  // def_vm_access_ok
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define USER_UID 1

static int daemon_pid = -1, thief_pid = -1;

static void sleeper(void) {
    while (1)
        sys_sleep(1000);
}

static int simulate(void) {
    ui_printf("  daemon (root) reads its private key, then setuid(%d)\n", USER_UID);
    ui_printf("  the key is still in its memory; the process is user %d's now\n", USER_UID);
    ui_printf("  user %d reads it: debugger, process_vm_readv, core file\n", USER_UID);
    return 1;
}

static int launch(void) {
    daemon_pid = process_create("daemon", sleeper, 0);
    thief_pid  = process_create("thief", sleeper, USER_UID);
    process_t *daemon = daemon_pid > 0 ? process_get_by_pid(daemon_pid) : 0;
    process_t *thief  = thief_pid > 0 ? process_get_by_pid(thief_pid) : 0;
    if (!daemon || !thief) {
        ui_printf("  could not start the processes\n");
        return 0;
    }
    uint32_t key = USERTASK_STACK;              // a user page the "key" sits in

    // setuid(1), as sys_setuid does it.
    int dumpable = def_dumpable_on_setuid(daemon, daemon->uid, USER_UID);
    daemon->uid = USER_UID;
    ui_printf("  daemon PID %d is user %d now; dumpable = %d\n", daemon_pid, USER_UID, dumpable);

    int read = def_vm_access_ok(thief, daemon, key, 16);
    ui_printf("  thief (user %d) reads the key at %x   ", USER_UID, key);
    ui_color(read ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", read ? "allowed: the key is out!" : "refused");
    ui_reset();
    return read;
}

static int cleanup(void) {
    int killed = 0;
    if (daemon_pid > 0 && process_get_by_pid(daemon_pid)) killed += process_kill(daemon_pid);
    if (thief_pid > 0 && process_get_by_pid(thief_pid)) killed += process_kill(thief_pid);
    daemon_pid = thief_pid = -1;
    return killed;
}

static const attack_t attack = {
    "core_dump_leak",
    "after root drops to a user, read the secrets it still holds",
    INV_DISCLOSURE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "dumpable_flag"
};

void atk_core_dump_leak_register(void) { registry_add_attack(&attack); }
