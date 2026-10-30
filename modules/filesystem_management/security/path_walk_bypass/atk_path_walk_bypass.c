/*
 * atk_path_walk_bypass.c - ATTACK path_walk_bypass: reach a file through
 * a private directory by a side road.
 *
 * Breaks:     privilege - a private directory hides everything below it
 * Stopped by: search_check (defence/search_check)
 *
 * root's /seclab_vault (mode 60) holds "gold". The plain path is
 * refused; an attacker tries the side roads:
 *   1. the full path                   /seclab_vault/gold
 *   2. in and out with ".."            /home/../seclab_vault/gold
 *   3. a directory descriptor opened while the vault was still open,
 *      then fstatat(fd, "gold") and fchdir(fd)
 *
 * simulate(): list the roads.
 * launch():   a user 1 process opens the vault while it is mode 64,
 *             root then sets 60, the process tries all four with the
 *             real syscalls. findings = roads that reached the file.
 * cleanup():  delete /seclab_vault.
 *
 * Uses: sys_stat / sys_fstatat / sys_fchdir (syscalls/), sys_open /
 *       sys_close (io_management), ramdisk_create / ramdisk_delete
 *       (core/ramdisk), process_create (process_management), sys_sleep, ui.
 */
#include "modules/security_management/core/registry/registry.h"            // attack_t, registry_add_attack
#include "modules/filesystem_management/syscalls/stat/sys_stat.h"          // sys_stat, vfs_stat_t
#include "modules/filesystem_management/syscalls/fstatat/sys_fstatat.h"    // sys_fstatat
#include "modules/filesystem_management/syscalls/fchdir/sys_fchdir.h"      // sys_fchdir
#include "modules/io_management/syscalls/open/sys_open.h"                  // sys_open, O_READ, O_DIRECTORY
#include "modules/io_management/syscalls/close/sys_close.h"                // sys_close
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"            // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"                    // fs_root, vfs_finddir
#include "modules/process_management/core/process/process.h"               // process_create
#include "modules/process_management/syscalls/sleep/sys_sleep.h"           // sys_sleep
#include "lib/string/string.h"                                             // strcpy
#include "modules/ui/core/style/ui.h"                                      // ui_printf, ui_color

#define VAULT    "seclab_vault"
#define ATTACKER 1
#define ROADS    4

static const char *const roads[ROADS] = {
    "stat(/seclab_vault/gold)", "stat(/home/../seclab_vault/gold)",
    "fstatat(old dirfd, gold)", "fchdir(old dirfd)",
};
static volatile int result[ROADS], opened, may_try, done;

static void walker(void) {
    vfs_stat_t st;
    int fd = sys_open("/" VAULT, O_READ | O_DIRECTORY);
    opened = 1;
    while (!may_try)
        sys_sleep(5);                               // root closes the vault now
    result[0] = sys_stat("/" VAULT "/gold", &st);
    result[1] = sys_stat("/home/../" VAULT "/gold", &st);
    result[2] = fd >= 0 ? sys_fstatat(fd, "gold", &st, 0) : -1;
    result[3] = fd >= 0 ? sys_fchdir(fd) : -1;
    if (fd >= 0)
        sys_close(fd);
    done = 1;
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
    for (int i = 0; i < ROADS; i++)
        ui_printf("  %s\n", roads[i]);
    return ROADS;
}

static int launch(void) {
    vfs_node_t *vault = vfs_finddir(fs_root, VAULT);
    if (!vault)
        vault = ramdisk_create(fs_root, VAULT, FS_DIRECTORY);
    if (!vault || (!vfs_finddir(vault, "gold") && !ramdisk_create(vault, "gold", FS_FILE)))
        return 0;
    vault->mode = 064;                              // open, for now
    opened = may_try = done = 0;
    if (process_create("walker", walker, ATTACKER) < 0)
        return 0;
    for (int i = 0; i < 50 && !opened; i++)
        sys_sleep(10);
    vault->mode = 060;                              // root: private
    may_try = 1;
    for (int i = 0; i < 50 && !done; i++)
        sys_sleep(10);

    int reached = 0;
    for (int i = 0; i < ROADS; i++) {
        int ok = result[i] == 0;
        reached += ok;
        ui_printf("  user %d: %-34s ", ATTACKER, roads[i]);
        ui_color(ok ? UI_BAD : UI_GOOD);
        ui_printf("%s\n", ok ? "got through!" : "refused");
        ui_reset();
    }
    return reached;
}

static int cleanup(void) {
    if (!vfs_finddir(fs_root, VAULT))
        return 0;
    remove_tree(fs_root, VAULT);
    return 1;
}

static const attack_t attack = {
    "path_walk_bypass",
    "reach a file in a private directory by '..' or an old dirfd",
    INV_PRIVILEGE, SEV_HIGH,
    simulate, launch, cleanup,
    "search_check"
};

void atk_path_walk_bypass_register(void) { registry_add_attack(&attack); }
