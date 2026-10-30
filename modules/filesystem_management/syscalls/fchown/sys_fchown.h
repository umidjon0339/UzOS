#ifndef SYS_FCHOWN_H
#define SYS_FCHOWN_H
/* sys_fchown.h - fchown(fd, uid, gid): chown the file behind a descriptor. */
#include <stdint.h>
#define SYS_FCHOWN 109       // EAX=109, EBX=fd, ECX=uid, EDX=gid
int sys_fchown(int fd, uint32_t uid, uint32_t gid);
#endif
