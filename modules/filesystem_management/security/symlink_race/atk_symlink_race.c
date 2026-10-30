/*
 * atk_symlink_race.c - ATTACK symlink_race: make root write through a trap.
 *
 * Breaks:     trust - root writes where it means to, not where a user points
 * Stopped by: protected_symlinks (defence/protected_symlinks)
 *
 * A root program writes /seclab_shared/report every night. The directory
 * is shared (others may write in it). User 1 plants
 *     /seclab_shared/report -> /seclab_target
 * and root's next write follows the link and overwrites root's own file
 * /seclab_target - one user 1 could never write.
 *
 * simulate(): the trap.
 * launch():   build it (a user 1 process makes the link), then root
 *             opens /seclab_shared/report for writing, as the program
 *             would. findings = 1 if /seclab_target was overwritten.
 * cleanup():  delete both.
 *
 * Uses: sys_symlink (syscalls/symlink), sys_open / sys_write / sys_close
 *       (io_management), ramdisk_* (core/ramdisk), process_create,
 *       sys_sleep, ui.
 */
#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"   // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"           // fs_root, vfs_finddir, vfs_write
#include "modules/process_management/core/process/process.h"      // process_create
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "lib/string/string.h"                                    // strcpy, strncmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color
#include "modules/filesystem_management/syscalls/symlink/sys_symlink.h"   // sys_symlink
#include "modules/io_management/syscalls/open/sys_open.h"                   // sys_open, O_WRITE, O_TRUNC
#include "modules/io_management/syscalls/write/sys_write.h"                 // sys_write
#include "modules/io_management/syscalls/close/sys_close.h"                 // sys_close

#define SHARED   "seclab_shared"
#define TARGET   "seclab_target"
#define ATTACKER 1

static volatile int planted;

static void plant(void) {
    planted = sys_symlink("/" TARGET, "/" SHARED "/report") == 0 ? 1 : -1;
}

// Delete 'name' in 'dir' and everything below it, deepest first.
static void remove_tree(vfs_node_t *dir, const char *name) {
    vfs_node_t *node = vfs_finddir(dir, name);
    if (!node)
        return;
    while (node->flags & FS_DIRECTORY && node->num_children > 0) {
        char child[FS_NAME_MAX + 1];
        strcpy(child, node->children[0]->name);
        remove_tree(node, child);
        if (vfs_finddir(node, child))
            break;                                  // could not: give up
    }
    ramdisk_delete(dir, name);
}

static int simulate(void) {
    ui_printf("  root writes /%s/report every night; others may write in /%s\n", SHARED, SHARED);
    ui_printf("  user %d plants report -> /%s; root overwrites its own file\n", ATTACKER, TARGET);
    return 1;
}

static int launch(void) {
    vfs_node_t *shared = vfs_finddir(fs_root, SHARED);
    if (!shared)
        shared = ramdisk_create(fs_root, SHARED, FS_DIRECTORY);
    vfs_node_t *target = vfs_finddir(fs_root, TARGET);
    if (!target)
        target = ramdisk_create(fs_root, TARGET, FS_FILE);
    if (!shared || !target)
        return 0;
    shared->mode = 066;                             // a shared directory, like /tmp
    vfs_write(target, 0, 9, (uint8_t *)"ROOT DATA");

    planted = 0;
    if (process_create("planter", plant, ATTACKER) < 0)
        return 0;
    for (int i = 0; i < 50 && !planted; i++)
        sys_sleep(10);
    ui_printf("  user %d planted /%s/report -> /%s: %s\n", ATTACKER, SHARED, TARGET,
              planted == 1 ? "yes" : "could not");

    int fd = sys_open("/" SHARED "/report", O_WRITE | O_TRUNC);    // root's nightly write
    if (fd >= 0) {
        sys_write(fd, "report", 6);
        sys_close(fd);
    }
    uint8_t now[9];
    vfs_read(target, 0, 9, now);
    int overwritten = target->size != 9 || strncmp((const char *)now, "ROOT DATA", 9) != 0;
    ui_printf("  root opens /%s/report for writing   ", SHARED);
    ui_color(overwritten ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", overwritten ? "followed: /seclab_target overwritten!" : "refused: the link is not followed");
    ui_reset();
    return overwritten;
}

static int cleanup(void) {
    int done = 0;
    if (vfs_finddir(fs_root, SHARED)) { remove_tree(fs_root, SHARED); done++; }
    if (vfs_finddir(fs_root, TARGET)) { ramdisk_delete(fs_root, TARGET); done++; }
    return done;
}

static const attack_t attack = {
    "symlink_race",
    "plant a link in a shared directory; root writes through it",
    INV_TRUST, SEV_HIGH,
    simulate, launch, cleanup,
    "protected_symlinks"
};

void atk_symlink_race_register(void) { registry_add_attack(&attack); }
