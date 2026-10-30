#ifndef SYS_SETHOSTNAME_H
#define SYS_SETHOSTNAME_H
/* sys_sethostname.h - sethostname(name, len): rename this machine. */
#include <stdint.h>
#define SYS_SETHOSTNAME 162   // EAX=162, EBX=name, ECX=len
// 0, or -errno: -EPERM (needs CAP_NET_ADMIN), -EINVAL (bad name or len).
int sys_sethostname(const char *name, uint32_t len);
#endif
