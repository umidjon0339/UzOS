#ifndef DEF_TIME_OWNER_H
#define DEF_TIME_OWNER_H

#include "modules/filesystem_management/core/vfs/vfs.h"   // vfs_node_t

// May the caller change 'node's times (utime, utimes, utimensat)?
//   to chosen values ('explicit' = 1): only its owner, or CAP_FILES
//   to "now"         ('explicit' = 0): also anyone who may write it
// 1 = allowed, 0 = refused (logged).
int def_utime_ok(const vfs_node_t *node, int explicit_times);

#endif
