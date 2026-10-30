/*
 * atk_symlink_loop.c - ATTACK symlink_loop: make a path walk go round for ever.
 *
 * Breaks:     liveness - every path walk ends
 * Stopped by: link_depth (defence/link_depth)
 *
 *   ln -s b a ; ln -s a b ; cat a      a -> b -> a -> b ...
 * Each link is followed by calling the walker again: a loop never ends
 * and a long chain runs off the kernel stack.
 *
 * simulate(): explain.
 * launch():   build a <-> b and a chain of 12 links in /seclab_loop, then
 *             stat() each. findings = walks that did not end in ELOOP.
 * cleanup():  delete /seclab_loop.
 *
 * Uses: sys_symlink / sys_stat (syscalls/), ramdisk_* (core/ramdisk),
 *       format (lib/format), ui.
 */
#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"   // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"           // fs_root, vfs_finddir, vfs_write
#include "modules/process_management/core/process/process.h"      // process_create
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "lib/string/string.h"                                    // strcpy, strncmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color
#include "modules/filesystem_management/syscalls/symlink/sys_symlink.h"   // sys_symlink
#include "modules/filesystem_management/syscalls/stat/sys_stat.h"           // sys_stat
#include "lib/format/format.h"                                              // format

#define TOP   "/seclab_loop"
#define CHAIN 12

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
    ui_printf("  a -> b -> a -> ...   and a chain of %d links\n", CHAIN);
    return 2;
}

static int launch(void) {
    if (!vfs_finddir(fs_root, "seclab_loop") && !ramdisk_create(fs_root, "seclab_loop", FS_DIRECTORY))
        return 0;
    sys_symlink("b", TOP "/a");
    sys_symlink("a", TOP "/b");
    char from[32], to[32];
    for (int i = 0; i < CHAIN; i++) {               // c0 -> c1 -> ... -> c12 (a file)
        format(from, sizeof(from), TOP "/c%d", i);
        format(to, sizeof(to), "c%d", i + 1);
        sys_symlink(to, from);
    }
    format(from, sizeof(from), "c%d", CHAIN);
    ramdisk_create(vfs_finddir(fs_root, "seclab_loop"), from, FS_FILE);

    vfs_stat_t st;
    int loop  = sys_stat(TOP "/a", &st) == 0;
    int chain = sys_stat(TOP "/c0", &st) == 0;
    ui_printf("  stat(a) with a <-> b        ");
    ui_color(loop ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", loop ? "resolved?!" : "ELOOP after 8 links");
    ui_reset();
    ui_printf("  stat(c0), 12 links long     ");
    ui_color(chain ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", chain ? "followed all of them!" : "ELOOP after 8 links");
    ui_reset();
    return loop + chain;
}

static int cleanup(void) {
    if (!vfs_finddir(fs_root, "seclab_loop"))
        return 0;
    remove_tree(fs_root, "seclab_loop");
    return 1;
}

static const attack_t attack = {
    "symlink_loop",
    "a <-> b, or a very long chain: the path walk never ends",
    INV_LIVENESS, SEV_MEDIUM,
    simulate, launch, cleanup,
    "link_depth"
};

void atk_symlink_loop_register(void) { registry_add_attack(&attack); }
