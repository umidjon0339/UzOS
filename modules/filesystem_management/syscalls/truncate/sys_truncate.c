/*
 * sys_truncate.c - truncate(path, length).
 *
 * Shorter: the end is cut off. Longer: the new bytes read as 0. Needs
 * write permission on the file, and a length within size_cap (64 KiB,
 * and the caller's RLIMIT_FSIZE). Returns 0 or -1.
 *
 * Uses: vfs_lookup / vfs_cwd (core/vfs), ramdisk_resize (core/ramdisk),
 *       perm_check (user_management), def_new_size_ok
 *       (security/huge_file/defence/size_cap).
 */
#include "modules/filesystem_management/syscalls/truncate/sys_truncate.h"   // this file
#include "modules/filesystem_management/core/vfs/vfs.h"                   // vfs_lookup, vfs_cwd
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"           // ramdisk_resize
#include "modules/user_management/core/perm/perm.h"                       // perm_check, PERM_WRITE
#include "modules/filesystem_management/security/huge_file/defence/size_cap/def_size_cap.h"  // def_new_size_ok

int sys_truncate(const char *path, uint32_t length) {
    vfs_node_t *file = vfs_lookup(vfs_cwd(), path);
    if (!file || !(file->flags & FS_FILE) || !perm_check(file, PERM_WRITE))
        return -1;
    // DEFENCE size_cap: no 2 GiB file in one call.
    if (!def_new_size_ok(length))
        return -1;
    return ramdisk_resize(file, length);
}
