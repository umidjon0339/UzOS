#ifndef DEF_DEPTH_LIMIT_H
#define DEF_DEPTH_LIMIT_H

#include <stdint.h>
#include "modules/filesystem_management/core/vfs/vfs.h"   // vfs_node_t

#define FS_DEPTH_MAX 32         // directories inside each other, at most

// How deep is 'node' (the root = 0)?
uint32_t def_depth_of(const vfs_node_t *node);

// May something 'height' levels tall (a new entry: 1; a moved directory:
// 1 + its deepest level) go inside 'dir'? ramdisk_create / ramdisk_move
// ask this. 1 = allowed, 0 = refused (logged).
int def_depth_ok(const vfs_node_t *dir, uint32_t height);

#endif
