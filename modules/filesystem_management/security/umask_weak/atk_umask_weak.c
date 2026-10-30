/*
 * atk_umask_weak.c - ATTACK umask_weak: make a privileged program create
 * files everyone may write.
 *
 * Breaks:     disclosure / integrity - root's files stay root's to change
 * Stopped by: secure_umask (defence/secure_umask)
 *
 * A program inherits its umask from whoever started it. Start a root
 * program with umask 0, and every file it creates - a log, a config, a
 * script it will run later - is writable by every user. One of them
 * edits the script; root runs it.
 *
 * simulate(): explain.
 * launch():   set the shell's umask to 0 (as an attacker's parent would),
 *             create /seclab_umask as root, look at its mode, put the
 *             umask back. findings = 1 if others may write the file.
 * cleanup():  delete /seclab_umask.
 *
 * Uses: ramdisk_create / ramdisk_delete (core/ramdisk), fs_root (core/vfs),
 *       the current process (process_management), perm_format
 *       (user_management), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"   // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"           // fs_root, vfs_finddir
#include "modules/process_management/core/scheduler/scheduler.h"  // scheduler_get_current_process
#include "modules/user_management/core/perm/perm.h"               // perm_format
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define FILE_NAME    "seclab_umask"
#define OTHERS_WRITE 002

static int simulate(void) {
    ui_printf("  a root program started with umask 0 creates its files rw/rw\n");
    ui_printf("  any user may then edit what root will read or run\n");
    return 1;
}

static int launch(void) {
    process_t *self = scheduler_get_current_process();
    uint32_t old = self->umask;
    self->umask = 0;                                // the attacker's umask
    vfs_node_t *f = ramdisk_create(fs_root, FILE_NAME, FS_FILE);
    self->umask = old;
    if (!f) {
        ui_printf("  could not create /%s (run `seclab repair`)\n", FILE_NAME);
        return 0;
    }
    char text[6];
    perm_format(f->mode, text);
    int writable = (f->mode & OTHERS_WRITE) != 0;
    ui_printf("  umask 0, root creates /%s: mode %s   ", FILE_NAME, text);
    ui_color(writable ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", writable ? "everyone may write it!" : "others may only read");
    ui_reset();
    return writable;
}

static int cleanup(void) {
    return vfs_finddir(fs_root, FILE_NAME) && ramdisk_delete(fs_root, FILE_NAME) == 0;
}

static const attack_t attack = {
    "umask_weak",
    "umask 0 makes a root program create world-writable files",
    INV_INTEGRITY, SEV_MEDIUM,
    simulate, launch, cleanup,
    "secure_umask"
};

void atk_umask_weak_register(void) { registry_add_attack(&attack); }
