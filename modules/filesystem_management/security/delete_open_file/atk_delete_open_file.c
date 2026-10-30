/*
 * atk_delete_open_file.c - ATTACK delete_open_file: free what is in use.
 *
 * Breaks:     consistency - nothing may point at freed memory
 * Stopped by: busy_check (security/defence/busy_check)
 *
 * Deleting frees the node. If a descriptor or a process's current
 * directory still points at it, the next kmalloc() hands that memory to
 * someone else, and the old pointer reads or writes their data.
 *
 * simulate(): list the three cases.
 * launch():   build them for real in / (an open file, a directory with a
 *             file inside, our own current directory) and try to delete
 *             each. Then clean everything up properly.
 *
 * Uses: ramdisk_* (core/ramdisk), vfs_open / vfs_close (core/vfs),
 *       sys_unlink / sys_rmdir (syscalls/), the current process (process_management).
 */

#include "modules/security_management/core/registry/registry.h"        // attack_t, registry_add_attack
#include "modules/security_management/core/audit/audit.h"              // audit_total
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"        // ramdisk_create
#include "modules/process_management/core/scheduler/scheduler.h"       // scheduler_get_current_process
#include "modules/filesystem_management/syscalls/unlink/sys_unlink.h"  // sys_unlink
#include "modules/filesystem_management/syscalls/rmdir/sys_rmdir.h"    // sys_rmdir
#include "modules/device_management/vga/vga.h"                         // vga_print_str

static int simulate(void) {
    vga_print_str("  delete a file that is still open (fd points at freed memory)\n");
    vga_print_str("  delete a directory that still has files (they leak)\n");
    vga_print_str("  delete the directory a process is standing in\n");
    return 3;
}

// unlink() for a file, rmdir() for a directory: either way busy_check decides.
static void try_delete(const char *what, const char *path, int is_dir) {
    vga_print_str("  ");
    vga_print_str(what);
    int deleted = (is_dir ? sys_rmdir(path) : sys_unlink(path)) == 0;
    vga_print_str(deleted ? " -> DELETED\n" : " -> refused\n");
}

static int launch(void) {
    uint32_t denied = audit_total();
    process_t *self = scheduler_get_current_process();

    // 1. An open file.
    vfs_node_t *file = ramdisk_create(fs_root, "seclab_open", FS_FILE);
    if (file) {
        vfs_open(file);
        try_delete("open file          ", "/seclab_open", 0);
        vfs_close(file);
        sys_unlink("/seclab_open");
    }

    // 2. A directory that is not empty.
    vfs_node_t *dir = ramdisk_create(fs_root, "seclab_dir", FS_DIRECTORY);
    if (dir && ramdisk_create(dir, "inside", FS_FILE)) {
        try_delete("non-empty directory", "/seclab_dir", 1);
        sys_unlink("/seclab_dir/inside");
    }

    // 3. The directory we are standing in.
    if (dir && self) {
        vfs_node_t *old_cwd = self->cwd;
        self->cwd = dir;
        try_delete("our current dir    ", "/seclab_dir", 1);
        self->cwd = old_cwd;
    }
    sys_rmdir("/seclab_dir");

    return audit_total() - denied;
}

static const attack_t attack = {
    "delete_open_file",
    "delete a file or directory that is still in use: use after free",
    INV_CONSISTENCY, SEV_CRITICAL,
    simulate, launch, 0,        // launch removes all its test entries
    "busy_check"
};

void atk_delete_open_file_register(void) {
    registry_add_attack(&attack);
}
