#ifndef SYS_FCHOWNAT_H
#define SYS_FCHOWNAT_H
/* sys_fchownat.h - fchownat(dirfd, path, uid, gid, flags): chown relative to a directory. */
#include <stdint.h>
#define SYS_FCHOWNAT 111       // EAX=111, EBX=dirfd, ECX=path, EDX=uid, ESI=gid, EDI=flags
int sys_fchownat(int dirfd, const char *path, uint32_t uid, uint32_t gid, int flags);
#endif
