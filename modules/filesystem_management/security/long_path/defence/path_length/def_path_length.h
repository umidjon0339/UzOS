#ifndef DEF_PATH_LENGTH_H
#define DEF_PATH_LENGTH_H

#include <stdint.h>
#include "modules/filesystem_management/core/vfs/vfs.h"   // vfs_node_t

// Would a new entry 'name' in directory 'dir' (or a moved subtree whose
// deepest path below its top is 'below' bytes) have a full path longer
// than FS_PATH_MAX - 1? ramdisk_create / ramdisk_move ask this.
// 1 = it fits, 0 = refused (logged).
int def_path_fits(const vfs_node_t *dir, const char *name, uint32_t below);

#endif
