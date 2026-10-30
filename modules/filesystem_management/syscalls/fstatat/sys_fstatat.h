#ifndef SYS_FSTATAT_H
#define SYS_FSTATAT_H
/* sys_fstatat.h - fstatat(dirfd, path, buf, flags): stat relative to a directory. */
#include "modules/filesystem_management/core/vfs/vfs.h"     // vfs_stat_t, AT_FDCWD
#define SYS_FSTATAT 100     // EAX=100, EBX=dirfd, ECX=path, EDX=buf, ESI=flags
#define AT_SYMLINK_NOFOLLOW 0x100   // describe a final symbolic link itself
#define AT_EMPTY_PATH       0x1000  // path "": stat dirfd itself
int sys_fstatat(int dirfd, const char *path, vfs_stat_t *buf, int flags);
#endif
