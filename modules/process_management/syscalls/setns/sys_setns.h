#ifndef SYS_SETNS_H
#define SYS_SETNS_H
/* sys_setns.h - setns(fd, nstype): join another process's namespace. */
#include <stdint.h>
#define SYS_SETNS 92            // EAX=92, EBX=fd, ECX=nstype
int sys_setns(int fd, uint32_t nstype);
#endif
