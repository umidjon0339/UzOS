#ifndef SYS_SETSID_H
#define SYS_SETSID_H
/* sys_setsid.h - setsid(): start a new session, caller becomes leader. */
#include <stdint.h>
#define SYS_SETSID 56       // EAX=56
int sys_setsid(void);
#endif
