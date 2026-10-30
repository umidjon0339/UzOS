#ifndef SYS_LSTAT_H
#define SYS_LSTAT_H
/* sys_lstat.h - lstat(path, buf): stat, but a symbolic link describes itself. */
#include "modules/filesystem_management/core/vfs/vfs.h"     // vfs_stat_t
#define SYS_LSTAT 98        // EAX=98, EBX=path, ECX=buf
int sys_lstat(const char *path, vfs_stat_t *buf);
#endif
