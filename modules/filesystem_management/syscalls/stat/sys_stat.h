#ifndef SYS_STAT_H
#define SYS_STAT_H
/* sys_stat.h - stat(path, buf): everything about a file but its bytes. */
#include "modules/filesystem_management/core/vfs/vfs.h"     // vfs_stat_t
#define SYS_STAT 97         // EAX=97, EBX=path, ECX=buf (vfs_stat_t)
int sys_stat(const char *path, vfs_stat_t *buf);
#endif
