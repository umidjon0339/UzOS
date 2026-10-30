/*
 * def_inode_refcount.c - DEFENCE inode_refcount: a file lives while
 * anything still refers to it.
 *
 * Stops:  use_after_unlink (security/use_after_unlink)
 * Called: ramdisk_delete() (core/ramdisk), when a name goes away, and
 *         vfs_close() (core/vfs), when a descriptor does.
 * Rule:   the file's memory is freed only when BOTH counts are 0: names
 *         (nlink) and open descriptors (open_count). Removing one name of
 *         a file that has others only removes that name.
 * Why:    with hard links, a file is not "its name": it is the inode the
 *         names point at. Freeing it when ONE name is removed leaves the
 *         other names, and every descriptor opened through them, pointing
 *         at freed kernel memory - a use-after-free: reading it shows
 *         whatever the heap holds next, writing it corrupts the heap.
 *         Counting every reference is how every Unix decides.
 *
 * Uses: registry (security_management). (A decision to keep is not a
 *       refusal: nothing is logged.)
 */
#include "modules/filesystem_management/security/use_after_unlink/defence/inode_refcount/def_inode_refcount.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence

static int free_now(uint32_t nlink, uint32_t open_count) {
    return nlink == 0 && open_count == 0;
}

int def_inode_free_ok(const vfs_node_t *inode) {
    return inode && free_now(inode->nlink, inode->open_count);
}

static int selftest(void) {
    if (!free_now(0, 0)) return 0;              // nothing refers to it: free
    if (free_now(1, 0)) return 0;               // another name: keep
    if (free_now(0, 1)) return 0;               // still open: keep
    if (free_now(2, 3)) return 0;
    return 1;
}

static const defence_t defence = {
    "inode_refcount",
    "a file is freed only at 0 names AND 0 open descriptors",
    "use_after_unlink",
    selftest
};

void def_inode_refcount_register(void) { registry_add_defence(&defence); }
