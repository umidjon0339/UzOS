/*
 * atk_nlink_overflow.c - ATTACK nlink_overflow: push the link count over its top.
 *
 * Breaks:     consistency - nlink says truly how many names a file has
 * Stopped by: nlink_bounds (defence/nlink_bounds)
 *
 * nlink decides when a file is freed (0 names, 0 fds). A counter that
 * can grow without end wraps: with 16-bit counts, 65536 links read as 0,
 * and the next unlink frees a file 65535 names still point at.
 *
 * simulate(): explain.
 * launch():   in /seclab_links, make 24 hard links to "file" with the
 *             real link(). findings = names beyond FS_LINK_MAX.
 * cleanup():  delete /seclab_links.
 *
 * Uses: sys_link (syscalls/link), ramdisk_* (core/ramdisk), format, ui.
 */
#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"   // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"           // fs_root, vfs_finddir, vfs_write
#include "modules/process_management/core/process/process.h"      // process_create
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "lib/string/string.h"                                    // strcpy, strncmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color
#include "modules/filesystem_management/syscalls/link/sys_link.h"       // sys_link
#include "modules/filesystem_management/security/nlink_overflow/defence/nlink_bounds/def_nlink_bounds.h"  // FS_LINK_MAX
#include "lib/format/format.h"                                          // format

#define TOP   "seclab_links"
#define TRIES 24

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
    ui_printf("  ln file l1 ; ln file l2 ; ... until the count wraps\n");
    ui_printf("  a file may have %d names\n", FS_LINK_MAX);
    return 1;
}

static int launch(void) {
    vfs_node_t *dir = vfs_finddir(fs_root, TOP);
    if (!dir)
        dir = ramdisk_create(fs_root, TOP, FS_DIRECTORY);
    vfs_node_t *file = dir ? (vfs_finddir(dir, "file") ? vfs_finddir(dir, "file") : ramdisk_create(dir, "file", FS_FILE)) : 0;
    if (!file)
        return 0;
    char name[32];
    int made = 0;
    for (int i = 0; i < TRIES; i++) {
        format(name, sizeof(name), "/" TOP "/l%d", i);
        made += sys_link("/" TOP "/file", name) == 0;
    }
    int extra = file->nlink > FS_LINK_MAX ? (int)(file->nlink - FS_LINK_MAX) : 0;
    ui_printf("  %d tries: %d links made; the file has %u names\n", TRIES, made, file->nlink);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "past the limit: the count can wrap" : "refused at 16 (EMLINK)");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    if (!vfs_finddir(fs_root, TOP))
        return 0;
    remove_tree(fs_root, TOP);
    return 1;
}

static const attack_t attack = {
    "nlink_overflow",
    "hard-link one file until its name count wraps around",
    INV_CONSISTENCY, SEV_MEDIUM,
    simulate, launch, cleanup,
    "nlink_bounds"
};

void atk_nlink_overflow_register(void) { registry_add_attack(&attack); }
