/*
 * atk_deep_nesting.c - ATTACK deep_nesting: a tree deep enough to blow a
 * kernel stack.
 *
 * Breaks:     liveness - `find /` must not crash the kernel
 * Stopped by: depth_limit (defence/depth_limit)
 *
 * `mkdir d; cd d` in a loop costs nothing. But `tree`, `find`, `du` and
 * `rm -r` call themselves once per level, on a 4 KiB kernel stack: a few
 * thousand levels and the next walk runs off the stack into whatever
 * memory lies below it.
 *
 * simulate(): explain.
 * launch():   under /seclab_deep, nest "d" directories 40 times, as far
 *             as the kernel lets. findings = levels beyond FS_DEPTH_MAX.
 * cleanup():  delete /seclab_deep and everything below it.
 *
 * Uses: ramdisk_create / ramdisk_delete (core/ramdisk), vfs_finddir
 *       (core/vfs), def_depth_of / FS_DEPTH_MAX (its defence), strcpy, ui.
 */
#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"   // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"           // fs_root, vfs_finddir
#include "modules/filesystem_management/security/deep_nesting/defence/depth_limit/def_depth_limit.h"  // def_depth_of, FS_DEPTH_MAX
#include "lib/string/string.h"                                    // strcpy
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define TOP   "seclab_deep"
#define TRIES 40

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
    ui_printf("  mkdir d; cd d  - %d times: every tree walker recurses that deep\n", TRIES);
    ui_printf("  on a 4 KiB kernel stack\n");
    return 1;
}

static int launch(void) {
    vfs_node_t *dir = vfs_finddir(fs_root, TOP);
    if (!dir)
        dir = ramdisk_create(fs_root, TOP, FS_DIRECTORY);
    if (!dir)
        return 0;
    while (vfs_finddir(dir, "d"))
        dir = vfs_finddir(dir, "d");                // a second run: go to the bottom
    for (int i = 0; i < TRIES; i++) {
        vfs_node_t *next = ramdisk_create(dir, "d", FS_DIRECTORY);
        if (!next)
            break;
        dir = next;
    }
    uint32_t depth = def_depth_of(dir);
    int beyond = depth > FS_DEPTH_MAX ? (int)(depth - FS_DEPTH_MAX) : 0;
    ui_printf("  the deepest directory is %u levels below /\n", depth);
    ui_color(beyond ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", beyond ? "deeper than any walker may go!" : "stopped at the limit (32)");
    ui_reset();
    return beyond;
}

static int cleanup(void) {
    if (!vfs_finddir(fs_root, TOP))
        return 0;
    remove_tree(fs_root, TOP);
    return 1;
}

static const attack_t attack = {
    "deep_nesting",
    "nest directories until a recursive walk overflows the kernel stack",
    INV_LIVENESS, SEV_HIGH,
    simulate, launch, cleanup,
    "depth_limit"
};

void atk_deep_nesting_register(void) { registry_add_attack(&attack); }
