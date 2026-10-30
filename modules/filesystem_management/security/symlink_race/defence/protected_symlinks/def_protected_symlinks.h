#ifndef DEF_PROTECTED_SYMLINKS_H
#define DEF_PROTECTED_SYMLINKS_H

#include "modules/filesystem_management/core/vfs/vfs.h"   // vfs_node_t

// May the caller follow symbolic link 'link', which lives in directory
// 'dir'? In a directory others may write to (a shared one, like /tmp),
// only a link owned by the caller or by the directory's owner is
// followed. vfs_lookup asks this. 1 = follow, 0 = refused (logged).
int def_follow_ok(const vfs_node_t *dir, const vfs_node_t *link);

#endif
