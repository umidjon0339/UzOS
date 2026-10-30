#ifndef SYS_CHOWN_H
#define SYS_CHOWN_H
/* sys_chown.h - chown(path, uid, gid): give a file to another user. */
#include <stdint.h>
#define SYS_CHOWN 108       // EAX=108, EBX=path, ECX=uid, EDX=gid
int sys_chown(const char *path, uint32_t uid, uint32_t gid);
#endif
