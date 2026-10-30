#ifndef DEF_NO_CYCLE_H
#define DEF_NO_CYCLE_H

#include "modules/filesystem_management/core/vfs/vfs.h"  // vfs_node_t

// May 'node' be moved into 'new_dir'? 1 = yes, 0 = refused (logged).
int def_move_allowed(vfs_node_t *node, vfs_node_t *new_dir);

#endif
