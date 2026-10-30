#ifndef DEF_DIR_READ_CHECK_H
#define DEF_DIR_READ_CHECK_H

#include "modules/filesystem_management/core/vfs/vfs.h"   // vfs_node_t

// May the caller list the entries of 'dir' NOW (getdents)? It needs
// read permission on the directory at the time of every listing - not
// just when the descriptor was opened. 1 = allowed, 0 = refused (logged).
int def_dir_list_ok(const vfs_node_t *dir);

#endif
