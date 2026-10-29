#ifndef SYS_WAITID_H
#define SYS_WAITID_H
/* sys_waitid.h - waitid(idtype, id): wait for a child by id selector. */
#include <stdint.h>
#define SYS_WAITID 64       // EAX=64, EBX=idtype, ECX=id
#define P_ALL 0             // wait for any child
#define P_PID 1             // wait for the child whose pid == id
int sys_waitid(int idtype, uint32_t id);
#endif
