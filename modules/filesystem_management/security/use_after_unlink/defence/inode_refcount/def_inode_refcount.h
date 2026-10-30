#ifndef DEF_INODE_REFCOUNT_H
#define DEF_INODE_REFCOUNT_H

#include "modules/filesystem_management/core/vfs/vfs.h"   // vfs_node_t

// May 'inode' (its bytes and its node) be freed NOW? Only when no name
// points at it (nlink 0) and no descriptor has it open (open_count 0).
// ramdisk_delete and vfs_close ask this. 1 = free it, 0 = keep it.
int def_inode_free_ok(const vfs_node_t *inode);

#endif
