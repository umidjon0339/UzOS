#ifndef SYS_GETPGRP_H
#define SYS_GETPGRP_H
/* sys_getpgrp.h - getpgrp(): the caller's own process-group id. */
#include <stdint.h>
#define SYS_GETPGRP 55      // EAX=55
uint32_t sys_getpgrp(void);
#endif
