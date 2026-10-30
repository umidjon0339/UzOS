/*
 * atk_timestamp_forgery.c - ATTACK timestamp_forgery: hide an edit by
 * setting the file's time back.
 *
 * Breaks:     integrity - a file's times are evidence
 * Stopped by: time_owner (defence/time_owner)
 *
 * "Who changed this, and when?" is answered from mtime and ctime. A user
 * who may WRITE a root file (it is shared) edits it, then sets its mtime
 * back to last year: the edit is hidden, and backup tools skip the file.
 *
 * simulate(): explain.
 * launch():   root's /seclab_evidence is writable by others (mode 66).
 *             A process of user 1 ("forger") asks utimensat for a chosen
 *             time (must be refused), then for "now" (allowed: a writer
 *             may). findings = 1 if the chosen time got through.
 * cleanup():  delete /seclab_evidence.
 *
 * Uses: sys_utimensat (syscalls/utimensat), ramdisk_create /
 *       ramdisk_delete (core/ramdisk), process_create (process_management),
 *       sys_sleep, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/filesystem_management/syscalls/utimensat/sys_utimensat.h"  // sys_utimensat, UTIME_NOW
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"   // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"           // fs_root, AT_FDCWD
#include "modules/process_management/core/process/process.h"      // process_create
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define FILE_NAME   "seclab_evidence"
#define ATTACKER    1
#define LAST_YEAR   1700000000u     // November 2023

static volatile int chosen_result, now_result, done;

static void forger(void) {
    utimensat_ts_t back[2] = { { LAST_YEAR, 0 }, { LAST_YEAR, 0 } };
    chosen_result = sys_utimensat(AT_FDCWD, "/" FILE_NAME, back, 0);
    now_result    = sys_utimensat(AT_FDCWD, "/" FILE_NAME, 0, 0);
    done = 1;
}

static int simulate(void) {
    ui_printf("  user %d may write root's shared file, edits it,\n", ATTACKER);
    ui_printf("  then sets its mtime back to 2023: the edit is hidden\n");
    return 1;
}

static int launch(void) {
    vfs_node_t *f = vfs_finddir(fs_root, FILE_NAME);
    if (!f)
        f = ramdisk_create(fs_root, FILE_NAME, FS_FILE);
    if (!f) {
        ui_printf("  could not create /%s\n", FILE_NAME);
        return 0;
    }
    f->mode = 066;                                  // others may write: shared
    done = 0;
    if (process_create("forger", forger, ATTACKER) < 0) {
        ui_printf("  could not start the forger\n");
        return 0;
    }
    for (int i = 0; i < 50 && !done; i++)
        sys_sleep(10);

    int forged = done && chosen_result == 0;
    ui_printf("  user %d: utimensat(chosen: 2023)   ", ATTACKER);
    ui_color(forged ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", forged ? "accepted: the edit is hidden!" : "refused (only the owner)");
    ui_reset();
    ui_printf("  user %d: utimensat(now)            %s\n", ATTACKER,
              now_result == 0 ? "allowed (a writer may)" : "refused");
    return forged;
}

static int cleanup(void) {
    return vfs_finddir(fs_root, FILE_NAME) && ramdisk_delete(fs_root, FILE_NAME) == 0;
}

static const attack_t attack = {
    "timestamp_forgery",
    "a writer of a shared file sets its mtime back to hide an edit",
    INV_INTEGRITY, SEV_MEDIUM,
    simulate, launch, cleanup,
    "time_owner"
};

void atk_timestamp_forgery_register(void) { registry_add_attack(&attack); }
