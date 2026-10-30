#ifndef DEF_PROTECTED_HARDLINKS_H
#define DEF_PROTECTED_HARDLINKS_H

#include "modules/filesystem_management/core/vfs/vfs.h"   // vfs_node_t

// May the caller make a new name (hard link) for 'inode'? Only for a
// file it owns (or with CAP_FILES), never for a directory. link() and
// linkat() ask this. 1 = allowed, 0 = refused (logged).
int def_hardlink_ok(const vfs_node_t *inode);

#endif
