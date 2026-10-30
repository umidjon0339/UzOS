#ifndef SYS_LCHOWN_H
#define SYS_LCHOWN_H
/* sys_lchown.h - lchown(path, uid, gid): chown, but a symbolic link itself. */
#include <stdint.h>
#define SYS_LCHOWN 110       // EAX=110, EBX=path, ECX=uid, EDX=gid
int sys_lchown(const char *path, uint32_t uid, uint32_t gid);
#endif
