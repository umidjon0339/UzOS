/*
 * atk_dirent_leak.c - ATTACK dirent_leak: list a directory after its
 * owner closed it.
 *
 * Breaks:     disclosure - the names in a private directory are private
 * Stopped by: dir_read_check (defence/dir_read_check)
 *
 * A descriptor lives on after the rights it was opened with. User 1
 * opens root's directory while it is readable; root then makes it
 * private (`chmod 60`). If getdents only trusted the open, user 1 would
 * keep reading every name that appears in it from then on.
 *
 * simulate(): explain.
 * launch():   root's /seclab_private (mode 64) holds "salaries". A user 1
 *             process opens it (O_DIRECTORY), root chmods it to 60, the
 *             process calls the real getdents64. findings = 1 if it got
 *             names after the chmod.
 * cleanup():  delete /seclab_private.
 *
 * Uses: sys_open / sys_seek / sys_close (io_management), sys_getdents64
 *       (syscalls/getdents64), ramdisk_create / ramdisk_delete (core/ramdisk),
 *       process_create (process_management), sys_sleep, ui.
 */
#include "modules/security_management/core/registry/registry.h"            // attack_t, registry_add_attack
#include "modules/filesystem_management/syscalls/getdents64/sys_getdents64.h"  // sys_getdents64
#include "modules/io_management/syscalls/open/sys_open.h"                  // sys_open, O_READ, O_DIRECTORY
#include "modules/io_management/syscalls/close/sys_close.h"                // sys_close
#include "modules/io_management/syscalls/seek/sys_seek.h"                  // sys_seek, SEEK_SET
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"            // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"                    // fs_root, vfs_finddir
#include "modules/process_management/core/process/process.h"               // process_create
#include "modules/process_management/syscalls/sleep/sys_sleep.h"           // sys_sleep
#include "lib/string/string.h"                                             // strcpy
#include "modules/ui/core/style/ui.h"                                      // ui_printf, ui_color

#define DIR_NAME "seclab_private"
#define ATTACKER 1

static volatile int opened, may_list, before, after, done;

static void reader(void) {
    uint8_t buf[256];
    int fd = sys_open("/" DIR_NAME, O_READ | O_DIRECTORY);
    before = fd >= 0 ? sys_getdents64(fd, buf, sizeof(buf)) : -1;
    opened = fd >= 0;
    while (!may_list)
        sys_sleep(5);                               // root changes the mode now
    if (fd >= 0) {
        sys_seek(fd, 0, SEEK_SET);                  // back to the first entry
        after = sys_getdents64(fd, buf, sizeof(buf));
        sys_close(fd);
    }
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
    ui_printf("  user %d opens root's directory while it is readable\n", ATTACKER);
    ui_printf("  root: chmod 60; user %d keeps listing it through the old descriptor\n", ATTACKER);
    return 1;
}

static int launch(void) {
    vfs_node_t *dir = vfs_finddir(fs_root, DIR_NAME);
    if (!dir)
        dir = ramdisk_create(fs_root, DIR_NAME, FS_DIRECTORY);
    if (!dir || (!vfs_finddir(dir, "salaries") && !ramdisk_create(dir, "salaries", FS_FILE))) {
        ui_printf("  could not build /%s\n", DIR_NAME);
        return 0;
    }
    dir->mode = 064;                                // readable by others, for now
    opened = may_list = done = 0;
    before = after = -1;
    if (process_create("lister", reader, ATTACKER) < 0)
        return 0;
    for (int i = 0; i < 50 && !opened; i++)
        sys_sleep(10);
    dir->mode = 060;                                // root: private from now on
    may_list = 1;
    for (int i = 0; i < 50 && !done; i++)
        sys_sleep(10);

    int leaked = after > 0;
    ui_printf("  mode 64: user %d lists it              %d bytes of names\n", ATTACKER, before);
    ui_printf("  chmod 60, same descriptor:            ");
    ui_color(leaked ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", leaked ? "still listed!" : "refused");
    ui_reset();
    return leaked;
}

static int cleanup(void) {
    if (!vfs_finddir(fs_root, DIR_NAME))
        return 0;
    remove_tree(fs_root, DIR_NAME);
    return 1;
}

static const attack_t attack = {
    "dirent_leak",
    "keep listing a directory through a descriptor after it went private",
    INV_DISCLOSURE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "dir_read_check"
};

void atk_dirent_leak_register(void) { registry_add_attack(&attack); }
