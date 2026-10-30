#ifndef SYS_FSTAT_H
#define SYS_FSTAT_H
/* sys_fstat.h - fstat(fd, buf): stat an open file. */
#include "modules/filesystem_management/core/vfs/vfs.h"     // vfs_stat_t
#define SYS_FSTAT 99        // EAX=99, EBX=fd, ECX=buf
int sys_fstat(int fd, vfs_stat_t *buf);
#endif
