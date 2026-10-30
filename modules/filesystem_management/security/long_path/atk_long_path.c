/*
 * atk_long_path.c - ATTACK long_path: build a file whose path cannot be written.
 *
 * Breaks:     resource - every path must fit a path buffer
 * Stopped by: path_length (defence/path_length)
 *
 * Names may be 127 bytes each and directories nest: three long names
 * deep, a node's full path is past 255 bytes. Then getcwd() fails, `pwd`
 * and the prompt cannot say where you are, and every kernel function
 * that builds a path must get truncation exactly right - a classic
 * place for an overflow.
 *
 * simulate(): the arithmetic.
 * launch():   under /seclab_long, create directories with 100-byte names
 *             inside each other, as far as the kernel lets. findings =
 *             nodes whose path no longer fits (vfs_path_of fails).
 * cleanup():  delete /seclab_long and everything below it.
 *
 * Uses: ramdisk_create / ramdisk_delete (core/ramdisk), vfs_path_of /
 *       vfs_finddir (core/vfs), memset / strcpy, ui.
 */
#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"   // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"           // fs_root, vfs_path_of, vfs_finddir
#include "lib/string/string.h"                                    // memset, strcpy
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define TOP      "seclab_long"
#define NAME_LEN 100
#define TRIES    5

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
    ui_printf("  names up to %d bytes, paths up to %d\n", FS_NAME_MAX, FS_PATH_MAX - 1);
    ui_printf("  /%s + 3 x 100-byte names = %d bytes: no buffer holds it\n", TOP, 12 + 3 * 101);
    return 1;
}

static int launch(void) {
    vfs_node_t *dir = vfs_finddir(fs_root, TOP);
    if (!dir)
        dir = ramdisk_create(fs_root, TOP, FS_DIRECTORY);
    if (!dir)
        return 0;
    char name[NAME_LEN + 1], path[FS_PATH_MAX];
    memset(name, 'x', NAME_LEN);
    name[NAME_LEN] = '\0';

    int made = 0, unprintable = 0;
    for (int i = 0; i < TRIES; i++) {
        name[0] = (char)('a' + i);
        vfs_node_t *next = ramdisk_create(dir, name, FS_DIRECTORY);
        if (!next)
            break;
        made++;
        unprintable += vfs_path_of(next, path, sizeof(path)) != 0;
        dir = next;
    }
    ui_printf("  100-byte directories made inside each other: %d (then refused)\n", made);
    ui_color(unprintable ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", unprintable ? "some have a path no buffer can hold!" : "every one still has a full path that fits");
    ui_reset();
    return unprintable;
}

static int cleanup(void) {
    if (!vfs_finddir(fs_root, TOP))
        return 0;
    remove_tree(fs_root, TOP);
    return 1;
}

static const attack_t attack = {
    "long_path",
    "nest long names until a path is longer than any path buffer",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "path_length"
};

void atk_long_path_register(void) { registry_add_attack(&attack); }
