#ifndef DEF_SEARCH_CHECK_H
#define DEF_SEARCH_CHECK_H

#include "modules/filesystem_management/core/vfs/vfs.h"   // vfs_node_t

// May the caller pass THROUGH directory 'dir' while walking a path (to a
// child, or to '..')? It needs read (UzOS's "search") permission on it -
// also on the directory a "*at" call starts from. vfs_lookup asks this
// for every directory on the way. 1 = yes, 0 = refused (logged).
int def_search_ok(const vfs_node_t *dir);

#endif
