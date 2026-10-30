#ifndef DEF_BUSY_CHECK_H
#define DEF_BUSY_CHECK_H

#include "modules/filesystem_management/core/vfs/vfs.h"  // vfs_node_t

// May 'node' be deleted now? 1 = yes, 0 = refused (logged).
int def_delete_allowed(vfs_node_t *node);

#endif
