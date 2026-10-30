#ifndef SYS_PAUSE_H
#define SYS_PAUSE_H
/* sys_pause.h - pause(): sleep until a signal comes. */
#define SYS_PAUSE 78            // EAX=78
int sys_pause(void);
#endif
