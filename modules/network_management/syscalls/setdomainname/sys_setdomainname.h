#ifndef SYS_SETDOMAINNAME_H
#define SYS_SETDOMAINNAME_H
/* sys_setdomainname.h - setdomainname(name, len): set the (NIS) domain name. */
#include <stdint.h>
#define SYS_SETDOMAINNAME 163   // EAX=163, EBX=name, ECX=len
// 0, or -errno: -EPERM (needs CAP_NET_ADMIN), -EINVAL.
int sys_setdomainname(const char *name, uint32_t len);
#endif
